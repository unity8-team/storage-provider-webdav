#include "RootsHandler.h"
#include "item_id.h"

#include <QDateTime>
#include <unity/storage/provider/metadata_keys.h>

using namespace std;
using namespace unity::storage::provider;
using unity::storage::ItemType;

namespace
{
const auto PROPFIND_BODY = QByteArrayLiteral(
R"(<?xml version="1.0" encoding="utf-8" ?>
<D:propfind xmlns:D="DAV:">
  <D:prop>
    <D:getetag/>
    <D:resourcetype/>
    <D:getcontentlength/>
    <D:creationdate/>
    <D:getlastmodified/>
  </D:prop>
</D:propfind>)");
}

RootsHandler::RootsHandler(DavProvider const& provider, Context const& ctx)
    : provider_(provider), base_url_(provider.base_url(ctx))
{
    QNetworkRequest request(base_url_);
    request.setRawHeader(QByteArrayLiteral("Depth"), QByteArrayLiteral("0"));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/xml; charset=\"utf-8\""));
    request.setHeader(QNetworkRequest::ContentLengthHeader, PROPFIND_BODY.size());
    request_body_.setData(PROPFIND_BODY);
    request_body_.open(QIODevice::ReadOnly);

    reply_.reset(provider.send_request(request, QByteArrayLiteral("PROPFIND"),
                                       &request_body_, ctx));
    if (!reply_)
    {
        sendError(RemoteCommsException("Could not make network request"));
        return;
    }
    connect(reply_.get(), static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &RootsHandler::onError);
    connect(reply_.get(), &QIODevice::readyRead,
            this, &RootsHandler::onReadyRead);
}

RootsHandler::~RootsHandler()
{
}

boost::future<unity::storage::provider::ItemList> RootsHandler::get_future()
{
    return promise_.get_future();
}

void RootsHandler::sendError(StorageException const& error)
{
    if (promise_set_) return;

    promise_.set_exception(error);
    promise_set_ = true;

    deleteLater();
}

void RootsHandler::sendResponse()
{
    if (promise_set_) return;

    promise_.set_value(roots_);
    promise_set_ = true;

    deleteLater();
}

void RootsHandler::onError(QNetworkReply::NetworkError code)
{
    // FIXME: include error info from code
    sendError(RemoteCommsException("Error from QNetworkReply"));
}

void RootsHandler::onReadyRead()
{
    disconnect(reply_.get(), &QIODevice::readyRead,
               this, &RootsHandler::onReadyRead);
    auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status != 207)
    {
        sendError(RemoteCommsException("Expected 207 response"));
        return;
    }
    parser_.reset(new MultiStatusParser(base_url_, reply_.get()));
    connect(parser_.get(), &MultiStatusParser::response,
            this, &RootsHandler::onParserResponse);
    connect(parser_.get(), &MultiStatusParser::finished,
            this, &RootsHandler::onParserFinished);
}

void RootsHandler::onParserResponse(QUrl const& href, std::vector<MultiStatusProperty> const& properties, int status)
{
    if (href != base_url_)
    {
        sendError(RemoteCommsException("PROPFIND returned information about a different URL"));
        return;
    }
    if (status != 0 && status != 200)
    {
        sendError(RemoteCommsException("Failed to look up properties for root: " + to_string(status)));
        return;
    }
    try
    {
        Item item;
        item.item_id = url_to_id(href, base_url_);
        item.name = "Root";
        item.type = ItemType::root;

        for (const auto& prop : properties)
        {
            if (prop.status != 200) continue;
            if (prop.ns == "DAV:")
            {
                if (prop.name == "getetag")
                {
                    item.etag = prop.value.toStdString();
                }
                else if (prop.name == "getcontentlength")
                {
                    item.metadata[SIZE_IN_BYTES] = static_cast<int64_t>(prop.value.toLongLong());
                }
                else if (prop.name == "creationdate")
                {
                    auto date = QDateTime::fromString(prop.value, Qt::RFC2822Date);
                    if (date.isValid())
                    {
                        item.metadata[CREATION_TIME] = date.toString(Qt::ISODate).toStdString();
                    }
                }
                else if (prop.name == "getlastmodified")
                {
                    auto date = QDateTime::fromString(prop.value, Qt::RFC2822Date);
                    if (date.isValid())
                    {
                        item.metadata[LAST_MODIFIED_TIME] = date.toString(Qt::ISODate).toStdString();
                    }
                }
            }
        }
        roots_.emplace_back(std::move(item));
    }
    catch (StorageException const& error)
    {
        sendError(error);
    }
    catch (exception const& error)
    {
        sendError(RemoteCommsException(string("Error creating item: ") + error.what()));
    }
}

void RootsHandler::onParserFinished()
{
    if (parser_->errorString().isEmpty())
    {
        sendResponse();
    }
    else
    {
        sendError(RemoteCommsException("Error parsing Multi-Status response: " + parser_->errorString().toStdString()));
    }
}
