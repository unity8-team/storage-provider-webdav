#include "PropFindHandler.h"
#include "item_id.h"
#include "http_error.h"

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
    : provider_(provider), base_url_(provider.base_url(ctx)), item_id_(item_id)
{
    QNetworkRequest request(id_to_url(item_id_, base_url_));
    request.setRawHeader(QByteArrayLiteral("Depth"), QByteArray::number(depth));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/xml; charset=\"utf-8\""));
    request.setHeader(QNetworkRequest::ContentLengthHeader, PROPFIND_BODY.size());
    request_body_.setData(PROPFIND_BODY);
    request_body_.open(QIODevice::ReadOnly);

    reply_.reset(provider.send_request(request, QByteArrayLiteral("PROPFIND"),
                                       &request_body_, ctx));
    assert(reply_.get() != nullptr);

    connect(reply_.get(), &QIODevice::readyRead,
            this, &PropFindHandler::onReplyReadyRead);
    connect(reply_.get(), &QNetworkReply::finished,
            this, &PropFindHandler::onReplyFinished);
}

PropFindHandler::~PropFindHandler()
{
}

void PropFindHandler::abort()
{
    if (finished_)
    {
        return;
    }
    finished_ = true;
    reply_->abort();
}

void PropFindHandler::reportError(StorageException const& error)
{
    reportError(boost::copy_exception(error));
}

void PropFindHandler::reportError(boost::exception_ptr const& ep)
{
    if (finished_)
    {
        return;
    }

    error_ = ep;
    finished_ = true;
    finish();
}

void PropFindHandler::reportSuccess()
{
    if (finished_)
    {
        return;
    }

    finished_ = true;
    finish();
}

void PropFindHandler::onReplyReadyRead()
{
    if (!seen_headers_)
    {
        seen_headers_ = true;
        auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 207)
        {
            disconnect(reply_.get(), &QIODevice::readyRead,
                       this, &PropFindHandler::onReplyReadyRead);
            parser_.reset(new MultiStatusParser(reply_->request().url(), reply_.get()));
            connect(parser_.get(), &MultiStatusParser::response,
                    this, &PropFindHandler::onParserResponse);
            connect(parser_.get(), &MultiStatusParser::finished,
                    this, &PropFindHandler::onParserFinished);
        }
        else
        {
            is_error_ = true;
        }
    }
    if (is_error_)
    {
        if (error_body_.size() < MAX_ERROR_BODY_LENGTH)
        {
            error_body_.append(reply_->readAll());
        }
        else
        {
            reply_->close();
        }
    }
}

void PropFindHandler::onReplyFinished()
{
    if (!seen_headers_ || is_error_)
    {
        reportError(translate_http_error(reply_.get(), error_body_, item_id_));
    }
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
