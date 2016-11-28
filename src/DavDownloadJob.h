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

#pragma once

#include <QByteArray>
#include <QLocalSocket>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <unity/storage/provider/Exceptions.h>
#include <unity/storage/provider/ProviderBase.h>
#include <unity/storage/provider/DownloadJob.h>

#include <cstdint>
#include <memory>
#include <string>

class DavProvider;

class DavDownloadJob : public QObject, public unity::storage::provider::DownloadJob
{
    Q_OBJECT
public:
    DavDownloadJob(std::shared_ptr<DavProvider> const& provider,
                   std::string const& item_id, std::string const& match_etag,
                   unity::storage::provider::Context const& ctx);
    ~DavDownloadJob();

    boost::future<void> cancel() override;
    boost::future<void> finish() override;

private Q_SLOTS:
    void onReplyFinished();
    void onReplyReadyRead();
    void onReplyReadChannelFinished();

    void onSocketBytesWritten(int64_t bytes);

private:
    void maybe_send_chunk();
    void handle_error(unity::storage::provider::StorageException const& exc);
    void handle_error(std::exception_ptr ep);

    std::shared_ptr<DavProvider> const provider_;
    std::string const item_id_;
    QLocalSocket writer_;
    std::unique_ptr<QNetworkReply> reply_;

    bool seen_header_ = false;
    bool read_channel_finished_ = false;
    int64_t bytes_read_ = 0;
    int64_t bytes_written_ = 0;

    bool is_error_ = false;
    QByteArray error_body_;
};
