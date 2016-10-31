#pragma once

#include <QObject>

#include <memory>

#include "PropFindHandler.h"

class LookupHandler : public PropFindHandler {
    Q_OBJECT
public:
    LookupHandler(DavProvider const& provider,
                  std::string const& item_id,
                  unity::storage::provider::Context const& ctx);
    ~LookupHandler();

    boost::future<unity::storage::provider::ItemList> get_future();

private:
    boost::promise<unity::storage::provider::ItemList> promise_;

protected:
    void finish() override;
};
