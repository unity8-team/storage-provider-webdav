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

#include "ListHandler.h"

#include <unity/storage/provider/Exceptions.h>

#include <algorithm>
#include <cassert>

using namespace std;
using namespace unity::storage::provider;

ListHandler::ListHandler(std::shared_ptr<DavProvider> const& provider,
                         string const& parent_id, Context const& ctx)
    : PropFindHandler(provider, parent_id, 1, ctx)
{
}

ListHandler::~ListHandler() = default;

boost::future<tuple<ItemList,string>> ListHandler::get_future()
{
    return promise_.get_future();
}

void ListHandler::finish()
{
    deleteLater();

    if (error_)
    {
        promise_.set_exception(error_);
        return;
    }
    // A "Depth: 1" PROPFIND will also return data for the parent URL
    // itself, so remove it from the list.
    items_.erase(remove_if(items_.begin(), items_.end(),
                           [&](Item const& item) -> bool {
                               return item.item_id == item_id_;
                           }), items_.end());

    promise_.set_value(make_tuple(move(items_), string()));
}
