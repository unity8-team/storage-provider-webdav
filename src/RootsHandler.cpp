#include "RootsHandler.h"

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

RootsHandler::RootsHandler(DavProvider const& provider, Context const& ctx)
    : PropFindHandler(provider, ".", 0, ctx)
{
}

RootsHandler::~RootsHandler() = default;

boost::future<ItemList> RootsHandler::get_future()
{
    return promise_.get_future();
}

void RootsHandler::finish()
{
    if (error_)
    {
        promise_.set_exception(error_);
        return;
    }
    // Perform some sanity checks on the result
    if (items_.size() != 1)
    {
        promise_.set_exception(RemoteCommsException("Unexpectedly received " + to_string(items_.size()) + " items from PROPFIND request"));
        return;
    }
    if (items_[0].item_id != ".")
    {
        promise_.set_exception(RemoteCommsException("PROPFIND request returned data about the wrong item"));
        return;
    }

    promise_.set_value(items_);
}
