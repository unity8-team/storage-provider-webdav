#pragma once

#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/Exceptions.h>

#include <memory>

#include "PropFindHandler.h"

class RootsHandler : public PropFindHandler {
    Q_OBJECT
public:
    RootsHandler(DavProvider const& provider,
                 unity::storage::provider::Context const& ctx);
    ~RootsHandler();

    boost::future<unity::storage::provider::ItemList> get_future();

private:
    boost::promise<unity::storage::provider::ItemList> promise_;

protected:
    void finish() override;
};
