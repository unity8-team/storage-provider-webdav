#pragma once

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
    DavDownloadJob(DavProvider const& provider, std::string const& item_id,
                   std::string const& match_etag,
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

    DavProvider const& provider_;
    std::string const item_id_;
    QLocalSocket writer_;
    std::unique_ptr<QNetworkReply> reply_;

    bool read_channel_finished_ = false;
    bool seen_header_ = false;
    int64_t bytes_read_ = 0;
    int64_t bytes_written_ = 0;
};
