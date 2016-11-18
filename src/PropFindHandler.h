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
#include <unity/storage/provider/Exceptions.h>

#include <memory>

#include "DavProvider.h"
#include "MultiStatusParser.h"

class PropFindHandler : public QObject {
    Q_OBJECT
public:
    PropFindHandler(std::shared_ptr<DavProvider> const& provider,
                    std::string const& item_id, int depth,
                    unity::storage::provider::Context const& ctx);
    ~PropFindHandler();

    void abort();

private Q_SLOTS:
    // From QNetworkReply
    void onReplyReadyRead();
    void onReplyFinished();

    // From MultiStatusParser
    void onParserResponse(QUrl const& href, std::vector<MultiStatusProperty> const& properties, int status);
    void onParserFinished();

private:
    void reportError(unity::storage::provider::StorageException const& error);
    void reportError(boost::exception_ptr const& ep);
    void reportSuccess();

    bool seen_headers_ = false;
    bool is_error_ = false;
    bool finished_ = false;
    boost::promise<unity::storage::provider::ItemList> promise_;

    std::shared_ptr<DavProvider> const provider_;
    QUrl base_url_;
    QBuffer request_body_;
    std::unique_ptr<QNetworkReply> reply_;
    std::unique_ptr<MultiStatusParser> parser_;
    QByteArray error_body_;

protected:
    virtual void finish() = 0;

    std::string const item_id_;
    unity::storage::provider::ItemList items_;
    boost::exception_ptr error_;
};
