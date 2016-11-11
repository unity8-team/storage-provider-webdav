#pragma once

#include <QBuffer>
#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/ProviderBase.h>

#include <memory>

class DavProvider;

class DeleteHandler : public QObject {
    Q_OBJECT
public:
    DeleteHandler(DavProvider const& provider, std::string const& item_id,
                  unity::storage::provider::Context const& ctx);
    ~DeleteHandler();

    boost::future<void> get_future();

private Q_SLOTS:
    void onFinished();

private:
    boost::promise<void> promise_;

    DavProvider const& provider_;
    std::string const item_id_;

    std::unique_ptr<QNetworkReply> reply_;
};
