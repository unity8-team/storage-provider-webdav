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

#include "DeleteHandler.h"
#include "DavProvider.h"
#include "item_id.h"
#include "http_error.h"

using namespace std;
using namespace unity::storage::provider;

DeleteHandler::DeleteHandler(shared_ptr<DavProvider> const& provider,
                             string const& item_id,
                             Context const& ctx)
    : provider_(provider), item_id_(item_id)
{
    QUrl const base_url = provider->base_url(ctx);
    QNetworkRequest request(id_to_url(item_id_, base_url));
    reply_.reset(provider->send_request(request, QByteArrayLiteral("DELETE"),
                                        nullptr, ctx));
    connect(reply_.get(), &QNetworkReply::finished,
            this, &DeleteHandler::onFinished);
}

DeleteHandler::~DeleteHandler() = default;

boost::future<void> DeleteHandler::get_future()
{
    return promise_.get_future();
}

void DeleteHandler::onFinished()
{
    auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status / 100 == 2)
    {
        promise_.set_value();
    }
    else
    {
        promise_.set_exception(
            translate_http_error(reply_.get(), QByteArray(), item_id_));
    }

    deleteLater();
}
