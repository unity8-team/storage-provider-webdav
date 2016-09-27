#include "RootsHandler.h"

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
    assert(reply_.get() != nullptr);

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
    if (promise_set_) {
        return;
    }

    promise_.set_exception(error);
    promise_set_ = true;

    deleteLater();
}

void RootsHandler::sendResponse()
{
    if (promise_set_) {
        return;
    }

    promise_.set_value(roots_);
    promise_set_ = true;

    deleteLater();
}

void RootsHandler::onError(QNetworkReply::NetworkError code)
{
    sendError(RemoteCommsException("Error from QNetworkReply: " +
                                   to_string(code)));
}

void RootsHandler::onReadyRead()
{
    disconnect(reply_.get(), &QIODevice::readyRead,
               this, &RootsHandler::onReadyRead);
    auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status != 207)
    {
        sendError(RemoteCommsException("Expected 207 response, but got " + to_string(status)));
        return;
    }
    parser_.reset(new MultiStatusParser(reply_->request().url(), reply_.get()));
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
        Item item = provider_.make_item(href, base_url_, properties);
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
