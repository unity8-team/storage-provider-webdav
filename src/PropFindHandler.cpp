#include "PropFindHandler.h"
#include "item_id.h"

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

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

PropFindHandler::PropFindHandler(DavProvider const& provider, string const& item_id, int depth, Context const& ctx)
    : provider_(provider), base_url_(provider.base_url(ctx))
{
    QNetworkRequest request(id_to_url(item_id, base_url_));
    request.setRawHeader(QByteArrayLiteral("Depth"), QByteArray::number(depth));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/xml; charset=\"utf-8\""));
    request.setHeader(QNetworkRequest::ContentLengthHeader, PROPFIND_BODY.size());
    request_body_.setData(PROPFIND_BODY);
    request_body_.open(QIODevice::ReadOnly);

    reply_.reset(provider.send_request(request, QByteArrayLiteral("PROPFIND"),
                                       &request_body_, ctx));
    assert(reply_.get() != nullptr);

    connect(reply_.get(), static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &PropFindHandler::onError);
    connect(reply_.get(), &QIODevice::readyRead,
            this, &PropFindHandler::onReadyRead);
}

PropFindHandler::~PropFindHandler()
{
}

void PropFindHandler::reportError(StorageException const& error)
{
    if (finished_)
    {
        return;
    }

    error_ = boost::copy_exception(error);
    finished_ = true;
    finish();

    deleteLater();
}

void PropFindHandler::reportSuccess()
{
    if (finished_)
    {
        return;
    }

    finished_ = true;
    finish();

    deleteLater();
}

void PropFindHandler::onError(QNetworkReply::NetworkError code)
{
    reportError(RemoteCommsException("Error from QNetworkReply: " +
                                     to_string(code)));
}

void PropFindHandler::onReadyRead()
{
    disconnect(reply_.get(), &QIODevice::readyRead,
               this, &PropFindHandler::onReadyRead);
    auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status != 207)
    {
        reportError(RemoteCommsException("Expected 207 response, but got " + to_string(status)));
        return;
    }
    parser_.reset(new MultiStatusParser(reply_->request().url(), reply_.get()));
    connect(parser_.get(), &MultiStatusParser::response,
            this, &PropFindHandler::onParserResponse);
    connect(parser_.get(), &MultiStatusParser::finished,
            this, &PropFindHandler::onParserFinished);
}

void PropFindHandler::onParserResponse(QUrl const& href, vector<MultiStatusProperty> const& properties, int status)
{
    if (status != 0 && status != 200)
    {
        reportError(RemoteCommsException("PROPFIND for " + href.toEncoded().toStdString() + " gave status " +  to_string(status)));
        return;
    }
    try
    {
        Item item = provider_.make_item(href, base_url_, properties);
        items_.emplace_back(move(item));
    }
    catch (StorageException const& error)
    {
        reportError(error);
    }
    catch (exception const& error)
    {
        reportError(RemoteCommsException(string("Error creating item: ") + error.what()));
    }
}

void PropFindHandler::onParserFinished()
{
    if (parser_->errorString().isEmpty())
    {
        reportSuccess();
    }
    else
    {
        reportError(RemoteCommsException("Error parsing Multi-Status response: " + parser_->errorString().toStdString()));
    }
}
