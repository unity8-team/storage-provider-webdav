#include "MetadataHandler.h"

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

MetadataHandler::MetadataHandler(DavProvider const& provider,
                                 string const& item_id, Context const& ctx)
    : PropFindHandler(provider, item_id, 0, ctx), item_id_(item_id)
{
}

MetadataHandler::~MetadataHandler() = default;

boost::future<Item> MetadataHandler::get_future()
{
    return promise_.get_future();
}

void MetadataHandler::finish()
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
    if (items_[0].item_id != item_id_)
    {
        promise_.set_exception(RemoteCommsException("PROPFIND request returned data about the wrong item"));
        return;
    }

    promise_.set_value(move(items_[0]));
}
