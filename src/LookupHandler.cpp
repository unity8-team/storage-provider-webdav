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

#include "LookupHandler.h"

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

LookupHandler::LookupHandler(std::shared_ptr<DavProvider> const& provider,
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
