#pragma once

#include <QObject>

#include <memory>

#include "PropFindHandler.h"

class RootsHandler : public PropFindHandler {
    Q_OBJECT
public:
    RootsHandler(std::shared_ptr<DavProvider> const& provider,
                 unity::storage::provider::Context const& ctx);
    ~RootsHandler();

    boost::future<unity::storage::provider::ItemList> get_future();

private:
    boost::promise<unity::storage::provider::ItemList> promise_;

protected:
    void finish() override;
};
