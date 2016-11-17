#include "RetrieveMetadataHandler.h"

#include <unity/storage/provider/Exceptions.h>

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

RetrieveMetadataHandler::RetrieveMetadataHandler(DavProvider const& provider,
                                                 string const& item_id,
                                                 Context const& ctx,
                                                 Callback callback)
    : PropFindHandler(provider, item_id, 0, ctx), callback_(callback)
{
}

RetrieveMetadataHandler::~RetrieveMetadataHandler() = default;

void RetrieveMetadataHandler::finish()
{
    Item item;
    boost::exception_ptr ex = error_;

    if (items_.size() != 1)
    {
        ex = boost::copy_exception(RemoteCommsException("Unexpectedly received " + to_string(items_.size()) + " items from PROPFIND request"));
    }
    else if (items_[0].item_id != item_id_)
    {
        ex = boost::copy_exception(RemoteCommsException("PROPFIND request returned data about the wrong item"));
    }
    else
    {
        item = items_[0];
    }
    callback_(item, ex);
}
