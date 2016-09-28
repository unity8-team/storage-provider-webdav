#include "LookupHandler.h"

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

LookupHandler::LookupHandler(DavProvider const& provider,
                             string const& item_id, Context const& ctx)
    : PropFindHandler(provider, item_id, 0, ctx)
{
}

LookupHandler::~LookupHandler() = default;

boost::future<ItemList> LookupHandler::get_future()
{
    return promise_.get_future();
}

void LookupHandler::finish()
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

    promise_.set_value(move(items_));
}
