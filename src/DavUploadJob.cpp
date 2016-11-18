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

#include "DavUploadJob.h"
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

string make_upload_id()
{
    static int counter = 0;
    return to_string(counter++);
}

}

DavUploadJob::DavUploadJob(shared_ptr<DavProvider> const& provider,
                           string const& item_id, int64_t size,
                           string const& content_type, bool allow_overwrite,
                           string const& old_etag, Context const& ctx)
    : QObject(), UploadJob(make_upload_id()), provider_(provider),
      item_id_(item_id), base_url_(provider->base_url(ctx)), size_(size),
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
        dup(read_socket()), QLocalSocket::ConnectedState, QIODevice::ReadOnly);
    reply_.reset(provider->send_request(
        request, QByteArrayLiteral("PUT"), &reader_, ctx));
    assert(reply_.get() != nullptr);
    connect(reply_.get(), &QNetworkReply::finished,
            this, &DavUploadJob::onReplyFinished);
}

DavUploadJob::~DavUploadJob() = default;

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
        promise_.set_exception(
            translate_http_error(reply_.get(), QByteArray(), item_id_));
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
