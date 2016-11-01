#include "DavUploadJob.h"
#include "DavProvider.h"
#include "RetrieveMetadataHandler.h"
#include "item_id.h"

#include <unity/storage/provider/Exceptions.h>
#include <unity/storage/provider/metadata_keys.h>

#include <cassert>
#include <cstdint>
#include <string>

using namespace std;
using namespace unity::storage::provider;
using unity::storage::ItemType;

namespace
{

string make_upload_id()
{
    static int counter = 0;
    return to_string(counter++);
}

}

DavUploadJob::DavUploadJob(DavProvider const& provider, string const& item_id,
                           int64_t size, string const& content_type,
                           bool allow_overwrite, string const& old_etag,
                           Context const& ctx)
    : QObject(), UploadJob(make_upload_id()), provider_(provider),
      item_id_(item_id), base_url_(provider.base_url(ctx)), size_(size),
      context_(ctx)
{
    QNetworkRequest request(id_to_url(item_id, base_url_));
    if (!content_type.empty())
    {
        request.setHeader(QNetworkRequest::ContentTypeHeader,
                          QByteArray::fromStdString(content_type));
    }
    if (!allow_overwrite)
    {
        request.setRawHeader(QByteArrayLiteral("If-None-Match"),
                             QByteArrayLiteral("*"));
    }
    if (!old_etag.empty())
    {
        request.setRawHeader(QByteArrayLiteral("If-Match"),
                             QByteArray::fromStdString(old_etag));
    }
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant::fromValue(size));
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, true);

    reader_.setSocketDescriptor(
        read_socket(), QLocalSocket::ConnectedState, QIODevice::ReadOnly);
    reply_.reset(provider.send_request(
        request, QByteArrayLiteral("PUT"), &reader_, ctx));
    assert(reply_.get() != nullptr);
    connect(reply_.get(), static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &DavUploadJob::onReplyError);
    connect(reply_.get(), &QNetworkReply::finished,
            this, &DavUploadJob::onReplyFinished);
}

DavUploadJob::~DavUploadJob() = default;

void DavUploadJob::onReplyError(QNetworkReply::NetworkError code)
{
    if (promise_set_)
    {
        return;
    }
    promise_.set_exception(RemoteCommsException("Error from QNetworkReply: " + to_string(code)));
    promise_set_ = true;
}

void DavUploadJob::onReplyFinished()
{
    if (promise_set_)
    {
        return;
    }
    auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    // Is this a success status code?
    if (status / 100 != 2)
    {
        promise_.set_exception(RemoteCommsException("Error from PUT: " + to_string(status)));
        promise_set_ = true;
        return;
    }
    // Queue up a PROPFIND request to retrieve the metadata for the upload.
    metadata_.reset(
        new RetrieveMetadataHandler(
            provider_, item_id_, context_,
            [this](Item const& item, boost::exception_ptr const& error) {
                if (promise_set_)
                {
                    return;
                }
                if (error)
                {
                    promise_.set_exception(error);
                }
                else
                {
                    promise_.set_value(item);
                }
                promise_set_ = true;
            }));
}

boost::future<void> DavUploadJob::cancel()
{
    if (!promise_set_)
    {
        if (metadata_)
        {
            metadata_->abort();
        }
        else
        {
            reply_->abort();
        }
    }
    return boost::make_ready_future();
}

boost::future<Item> DavUploadJob::finish()
{
    return promise_.get_future();
}
