#pragma once

#include <QBuffer>
#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/ProviderBase.h>

#include <memory>

class DavProvider;
class RetrieveMetadataHandler;

class CopyMoveHandler : public QObject {
    Q_OBJECT
public:
    CopyMoveHandler(std::shared_ptr<DavProvider> const& provider,
                    std::string const& item_id,
                    std::string const& new_parent_id,
                    std::string const& new_name,
                    bool copy,
                    unity::storage::provider::Context const& ctx);
    ~CopyMoveHandler();

    boost::future<unity::storage::provider::Item> get_future();

private Q_SLOTS:
    void onFinished();

private:
    boost::promise<unity::storage::provider::Item> promise_;

    std::shared_ptr<DavProvider> const provider_;
    std::string const item_id_;
    std::string const new_item_id_;
    unity::storage::provider::Context const context_;

    std::unique_ptr<QNetworkReply> reply_;
    std::unique_ptr<RetrieveMetadataHandler> metadata_;
};
