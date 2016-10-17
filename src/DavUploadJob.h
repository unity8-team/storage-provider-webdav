#pragma once

#include <QLocalSocket>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>
#include <unity/storage/provider/ProviderBase.h>
#include <unity/storage/provider/UploadJob.h>

#include <cstdint>
#include <memory>
#include <string>

class DavProvider;

class DavUploadJob : public QObject, public unity::storage::provider::UploadJob
{
    Q_OBJECT
public:
    DavUploadJob(DavProvider const& provider, std::string const& item_id,
                 int64_t size, std::string const& content_type,
                 bool allow_overwrite, std::string const& old_etag,
                 unity::storage::provider::Context const& ctx);
    ~DavUploadJob();

    boost::future<void> cancel() override;
    boost::future<unity::storage::provider::Item> finish() override;

private:
    DavProvider const& provider_;
    std::string const item_id_;
    QUrl const base_url_;
    int64_t const size_;
    QLocalSocket reader_;
    std::unique_ptr<QNetworkReply> reply_;
};
