#include "DavDownloadJob.h"
#include "DavProvider.h"
#include "RetrieveMetadataHandler.h"
#include "item_id.h"

#include <unity/storage/provider/Exceptions.h>
#include <unity/storage/provider/metadata_keys.h>

#include <unistd.h>
#include <cassert>
#include <cstdint>
#include <string>

using namespace std;
using namespace unity::storage::provider;
using unity::storage::ItemType;

namespace
{

string make_download_id()
{
    static int counter = 0;
    return to_string(counter++);
}

constexpr int CHUNK_SIZE = 64 * 1024;

}

DavDownloadJob::DavDownloadJob(DavProvider const& provider,
                               string const& item_id,
                               string const& match_etag,
                               Context const& ctx)
    : QObject(), DownloadJob(make_download_id()), provider_(provider),
      item_id_(item_id)
{
    QUrl base_url = provider.base_url(ctx);
    QNetworkRequest request(id_to_url(item_id, base_url));
    if (!match_etag.empty())
    {
        request.setRawHeader(QByteArrayLiteral("If-Match"),
                             QByteArray::fromStdString(match_etag));
    }

    reply_ = provider.send_request(
        request, QByteArrayLiteral("GET"), nullptr, ctx);
    assert(reply_ != nullptr);
    reply_->setReadBufferSize(CHUNK_SIZE);
    connect(reply_, &QNetworkReply::finished,
            this, &DavDownloadJob::onReplyFinished);
    connect(reply_, &QIODevice::readyRead,
            this, &DavDownloadJob::onReplyReadyRead);
    connect(reply_, &QIODevice::readChannelFinished,
            this, &DavDownloadJob::onReplyReadChannelFinished);
    writer_.setSocketDescriptor(
        dup(write_socket()), QLocalSocket::ConnectedState, QIODevice::WriteOnly);
    connect(&writer_, &QIODevice::bytesWritten,
            this, &DavDownloadJob::onSocketBytesWritten);
}

DavDownloadJob::~DavDownloadJob()
{
    if (!reply_.isNull())
    {
        reply_->deleteLater();
    }
}

void DavDownloadJob::onReplyFinished()
{
    // If we haven't seen HTTP response headers and are in an error
    // state, report that.
    if (!seen_header_ && reply_->error() != QNetworkReply::NoError)
    {
        handle_error(RemoteCommsException("Error connecting to server: " +
                                          reply_->errorString().toStdString()));
    }
}

void DavDownloadJob::onReplyReadyRead()
{
    if (!seen_header_)
    {
        seen_header_ = true;
        auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200)
        {
            handle_error(RemoteCommsException("Unexpected status code: " +
                                              to_string(status)));
            return;
        }
    }

    maybe_send_chunk();
}

void DavDownloadJob::onReplyReadChannelFinished()
{
    read_channel_finished_ = true;
    maybe_send_chunk();
}

void DavDownloadJob::onSocketBytesWritten(int64_t bytes)
{
    bytes_written_ += bytes;
    maybe_send_chunk();
}

void DavDownloadJob::maybe_send_chunk()
{
    assert(bytes_written_ <= bytes_read_);
    // If there are outstanding writes, do nothing.
    if (bytes_written_ < bytes_read_)
    {
        return;
    }
    // If there are no bytes available, return.
    if (reply_->bytesAvailable() == 0)
    {
        // If we've reached the end of the input, and all data has been
        // written out, signal completion.
        if (read_channel_finished_ && bytes_written_ == bytes_read_)
        {
            writer_.close();
            report_complete();
        }
        return;
    }

    char buffer[CHUNK_SIZE];
    int n_read = reply_->read(buffer, CHUNK_SIZE);
    if (n_read < 0)
    {
        handle_error(RemoteCommsException("Failed to read from server"));
        return;
    }
    bytes_read_ += n_read;

    int n_written = writer_.write(buffer, n_read);
    if (n_written < 0)
    {
        handle_error(ResourceException("Error writing to socket", 0));
        return;
    }
}

void DavDownloadJob::handle_error(StorageException const& exc)
{
    reply_->close();
    writer_.close();
    report_error(std::make_exception_ptr(exc));
}


boost::future<void> DavDownloadJob::cancel()
{
    reply_->abort();
    writer_.close();
    return boost::make_ready_future();
}

boost::future<void> DavDownloadJob::finish()
{
    return boost::make_exceptional_future<void>(
        LogicException("finish called before all data sent"));
}
