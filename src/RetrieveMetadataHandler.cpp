#include "RetrieveMetadataHandler.h"

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
    callback_(items_, error_);
}
