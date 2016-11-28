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

#include "CreateFolderHandler.h"
#include "RetrieveMetadataHandler.h"
#include "item_id.h"
#include "http_error.h"

using namespace std;
using namespace unity::storage::provider;

CreateFolderHandler::CreateFolderHandler(shared_ptr<DavProvider> const& provider,
                                         string const& parent_id,
                                         string const& name,
                                         Context const& ctx)
    : provider_(provider), item_id_(make_child_id(parent_id, name, true)),
      context_(ctx)
{
    QUrl const base_url = provider->base_url(ctx);
    QNetworkRequest request(id_to_url(item_id_, base_url));
    reply_.reset(provider->send_request(request, QByteArrayLiteral("MKCOL"),
                                        nullptr, ctx));
    connect(reply_.get(), &QNetworkReply::finished,
            this, &CreateFolderHandler::onFinished);
}

CreateFolderHandler::~CreateFolderHandler() = default;

boost::future<Item> CreateFolderHandler::get_future()
{
    return promise_.get_future();
}

void CreateFolderHandler::onFinished()
{
    auto status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status != 201)
    {
        promise_.set_exception(
            translate_http_error(reply_.get(), QByteArray(), item_id_));
        deleteLater();
        return;
    }

    metadata_.reset(
        new RetrieveMetadataHandler(
            provider_, item_id_, context_,
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
