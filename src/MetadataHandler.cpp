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

#include "MetadataHandler.h"

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

MetadataHandler::MetadataHandler(shared_ptr<DavProvider> const& provider,
                                 string const& item_id, Context const& ctx)
    : PropFindHandler(provider, item_id, 0, ctx)
{
}

MetadataHandler::~MetadataHandler() = default;

boost::future<Item> MetadataHandler::get_future()
{
    return promise_.get_future();
}

void MetadataHandler::finish()
{
    deleteLater();

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
