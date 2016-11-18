/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#include "RetrieveMetadataHandler.h"

#include <unity/storage/provider/Exceptions.h>

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

RetrieveMetadataHandler::RetrieveMetadataHandler(shared_ptr<DavProvider> const& provider,
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
