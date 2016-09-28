#pragma once

#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/Exceptions.h>

#include <memory>

#include "PropFindHandler.h"

class MetadataHandler : public PropFindHandler {
    Q_OBJECT
public:
    MetadataHandler(DavProvider const& provider,
                    std::string const& item_id,
                    unity::storage::provider::Context const& ctx);
    ~MetadataHandler();

    boost::future<unity::storage::provider::Item> get_future();

private:
    std::string const item_id_;
    boost::promise<unity::storage::provider::Item> promise_;

protected:
    void finish() override;
};
