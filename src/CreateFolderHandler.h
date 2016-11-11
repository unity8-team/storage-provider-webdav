#pragma once

#include <QBuffer>
#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/ProviderBase.h>

#include <memory>

class DavProvider;
class RetrieveMetadataHandler;

class CreateFolderHandler : public QObject {
    Q_OBJECT
public:
    CreateFolderHandler(DavProvider const& provider,
                        std::string const& parent_id, std::string const& name,
                        unity::storage::provider::Context const& ctx);
    ~CreateFolderHandler();

    boost::future<unity::storage::provider::Item> get_future();

private Q_SLOTS:
    void onFinished();

private:
    boost::promise<unity::storage::provider::Item> promise_;

    DavProvider const& provider_;
    std::string const item_id_;
    unity::storage::provider::Context const context_;

    std::unique_ptr<QNetworkReply> mkcol_;
    std::unique_ptr<RetrieveMetadataHandler> metadata_;
};