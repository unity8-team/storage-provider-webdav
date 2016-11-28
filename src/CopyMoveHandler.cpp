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

#include "CopyMoveHandler.h"
#include "RetrieveMetadataHandler.h"
#include "item_id.h"
#include "http_error.h"

using namespace std;
using namespace unity::storage::provider;

CopyMoveHandler::CopyMoveHandler(shared_ptr<DavProvider> const& provider,
                                 string const& item_id,
                                 string const& new_parent_id,
                                 string const& new_name,
                                 bool copy,
                                 Context const& ctx)
    : provider_(provider), item_id_(item_id),
      new_item_id_(make_child_id(new_parent_id, new_name, is_folder(item_id))),
      context_(ctx)
{
    QUrl const base_url = provider->base_url(ctx);
    QNetworkRequest request(id_to_url(item_id_, base_url));
    request.setRawHeader(QByteArrayLiteral("Destination"),
                         id_to_url(new_item_id_, base_url).toEncoded());
    // Error out of the operation would overwrite an existing resource.
    request.setRawHeader(QByteArrayLiteral("Overwrite"),
                         QByteArrayLiteral("F"));

    reply_.reset(provider->send_request(
        request, copy ? QByteArrayLiteral("COPY") : QByteArrayLiteral("MOVE"),
        nullptr, ctx));
    connect(reply_.get(), &QNetworkReply::finished,
            this, &CopyMoveHandler::onFinished);
}

CopyMoveHandler::~CopyMoveHandler() = default;

boost::future<Item> CopyMoveHandler::get_future()
{
    return promise_.get_future();
}

void CopyMoveHandler::onFinished()
{
    auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status != 201 && status != 204)
    {
        promise_.set_exception(
            translate_http_error(reply_.get(), QByteArray(), item_id_));
        deleteLater();
        return;
    }

    metadata_.reset(
        new RetrieveMetadataHandler(
            provider_, new_item_id_, context_,
            [this](Item const& item, boost::exception_ptr const& error) {
                if (error)
                {
                    promise_.set_exception(error);
                }
                else
                {
                    promise_.set_value(item);
                }
                deleteLater();
            }));
}
