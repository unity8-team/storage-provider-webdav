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

#pragma once

#include <QObject>

#include <memory>

#include "PropFindHandler.h"

class LookupHandler : public PropFindHandler {
    Q_OBJECT
public:
    LookupHandler(std::shared_ptr<DavProvider> const& provider,
                  std::string const& item_id,
                  unity::storage::provider::Context const& ctx);
    ~LookupHandler();

    boost::future<unity::storage::provider::ItemList> get_future();

private:
    boost::promise<unity::storage::provider::ItemList> promise_;

protected:
    void finish() override;
};
