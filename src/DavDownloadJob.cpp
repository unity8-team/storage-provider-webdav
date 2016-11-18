/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#include "DavDownloadJob.h"
#include "DavProvider.h"
#include "RetrieveMetadataHandler.h"
#include "item_id.h"
#include "http_error.h"

#include <unity/storage/provider/Exceptions.h>

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

DavDownloadJob::DavDownloadJob(shared_ptr<DavProvider> const& provider,
                               string const& item_id,
                               string const& match_etag,
                               Context const& ctx)
    : QObject(), DownloadJob(make_download_id()), provider_(provider),
      item_id_(item_id)
{
    QUrl base_url = provider->base_url(ctx);
    QNetworkRequest request(id_to_url(item_id, base_url));
    if (!match_etag.empty())
    {
        request.setRawHeader(QByteArrayLiteral("If-Match"),
                             QByteArray::fromStdString(match_etag));
    }

    reply_.reset(provider->send_request(
        request, QByteArrayLiteral("GET"), nullptr, ctx));
    assert(reply_.get() != nullptr);
    reply_->setReadBufferSize(CHUNK_SIZE);
    connect(reply_.get(), &QNetworkReply::finished,
            this, &DavDownloadJob::onReplyFinished);
    connect(reply_.get(), &QIODevice::readyRead,
            this, &DavDownloadJob::onReplyReadyRead);
    connect(reply_.get(), &QIODevice::readChannelFinished,
            this, &DavDownloadJob::onReplyReadChannelFinished);
    writer_.setSocketDescriptor(
        dup(write_socket()), QLocalSocket::ConnectedState, QIODevice::WriteOnly);
    connect(&writer_, &QIODevice::bytesWritten,
            this, &DavDownloadJob::onSocketBytesWritten);
}

DavDownloadJob::~DavDownloadJob() = default;

void DavDownloadJob::onReplyFinished()
{
    if (!seen_header_ || is_error_)
    {
        try
        {
            boost::rethrow_exception(
                translate_http_error(reply_.get(), error_body_, item_id_));
        }
        catch (...)
        {
            handle_error(std::current_exception());
        }
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
    else
    {
        maybe_send_chunk();
    }
}

void DavDownloadJob::onReplyReadChannelFinished()
{
    if (is_error_)
    {
        return;
    }

    read_channel_finished_ = true;
    maybe_send_chunk();
}

void DavDownloadJob::onSocketBytesWritten(int64_t bytes)
{
    if (is_error_)
    {
        return;
    }

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
        handle_error(RemoteCommsException("Failed to read from server: " +
                                          reply_->errorString().toStdString()));
        return;
    }
    bytes_read_ += n_read;

    int n_written = writer_.write(buffer, n_read);
    if (n_written < 0)
    {
        handle_error(ResourceException(
                         "Error writing to socket: "
                         + writer_.errorString().toStdString(), 0));
        return;
    }
}

void DavDownloadJob::handle_error(StorageException const& exc)
{
    handle_error(std::make_exception_ptr(exc));
}

void DavDownloadJob::handle_error(std::exception_ptr ep)
{
    is_error_ = true;
    reply_->close();
    writer_.close();
    report_error(ep);
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
