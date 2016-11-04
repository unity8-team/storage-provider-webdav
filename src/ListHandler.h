#pragma once

#include <QObject>

#include <memory>
#include <tuple>

#include "PropFindHandler.h"

class ListHandler : public PropFindHandler {
    Q_OBJECT
public:
    ListHandler(DavProvider const& provider,
                std::string const& parent_id,
                unity::storage::provider::Context const& ctx);
    ~ListHandler();

    boost::future<std::tuple<unity::storage::provider::ItemList,std::string>> get_future();

private:
    std::string const parent_id_;
    boost::promise<std::tuple<unity::storage::provider::ItemList,std::string>> promise_;

protected:
    void finish() override;
};
