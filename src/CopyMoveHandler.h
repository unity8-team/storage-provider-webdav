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

#include <QBuffer>
#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/ProviderBase.h>

#include <memory>

class DavProvider;
class RetrieveMetadataHandler;

class CopyMoveHandler : public QObject {
    Q_OBJECT
public:
    CopyMoveHandler(std::shared_ptr<DavProvider> const& provider,
                    std::string const& item_id,
                    std::string const& new_parent_id,
                    std::string const& new_name,
                    bool copy,
                    unity::storage::provider::Context const& ctx);
    ~CopyMoveHandler();

    boost::future<unity::storage::provider::Item> get_future();

private Q_SLOTS:
    void onFinished();

private:
    boost::promise<unity::storage::provider::Item> promise_;

    std::shared_ptr<DavProvider> const provider_;
    std::string const item_id_;
    std::string const new_item_id_;
    unity::storage::provider::Context const context_;

    std::unique_ptr<QNetworkReply> reply_;
    std::unique_ptr<RetrieveMetadataHandler> metadata_;
};
