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
#include <tuple>

#include "PropFindHandler.h"

class ListHandler : public PropFindHandler {
    Q_OBJECT
public:
    ListHandler(std::shared_ptr<DavProvider> const& provider,
                std::string const& parent_id,
                unity::storage::provider::Context const& ctx);
    ~ListHandler();

    boost::future<std::tuple<unity::storage::provider::ItemList,std::string>> get_future();

private:
    boost::promise<std::tuple<unity::storage::provider::ItemList,std::string>> promise_;

protected:
    void finish() override;
};
