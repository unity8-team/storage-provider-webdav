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

#include <functional>
#include <memory>

#include "PropFindHandler.h"

class RetrieveMetadataHandler : public PropFindHandler {
    Q_OBJECT
public:
    typedef std::function<void(unity::storage::provider::Item const& item,
                               boost::exception_ptr const& error)> Callback;

    RetrieveMetadataHandler(std::shared_ptr<DavProvider> const& provider,
                            std::string const& item_id,
                            unity::storage::provider::Context const& ctx,
                            Callback callback);
    ~RetrieveMetadataHandler();

private:
    Callback const callback_;

protected:
    void finish() override;
};
