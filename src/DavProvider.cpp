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

#include "DavProvider.h"
#include "MultiStatusParser.h"
#include "RootsHandler.h"
#include "ListHandler.h"
#include "LookupHandler.h"
#include "MetadataHandler.h"
#include "DavDownloadJob.h"
#include "DavUploadJob.h"
#include "CreateFolderHandler.h"
#include "DeleteHandler.h"
#include "CopyMoveHandler.h"
#include "item_id.h"

#include <QDateTime>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <unity/storage/common.h>
#include <unity/storage/provider/Exceptions.h>

using namespace std;
using namespace unity::storage::provider;
using namespace unity::storage::metadata;
using unity::storage::ItemType;

DavProvider::DavProvider()
    : network_(new QNetworkAccessManager)
{
}

DavProvider::~DavProvider() = default;

std::shared_ptr<DavProvider> DavProvider::shared_from_this()
{
    return static_pointer_cast<DavProvider>(ProviderBase::shared_from_this());
}

boost::future<ItemList> DavProvider::roots(
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    auto handler = new RootsHandler(shared_from_this(), ctx);
    return handler->get_future();
}

boost::future<tuple<ItemList,string>> DavProvider::list(
    string const& item_id, string const& page_token,
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    if (!page_token.empty())
    {
        throw InvalidArgumentException("Invalid paging token: " + page_token);
    }
    auto handler = new ListHandler(shared_from_this(), item_id, ctx);
    return handler->get_future();
}

boost::future<ItemList> DavProvider::lookup(
    string const& parent_id, string const& name,
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    string item_id = make_child_id(parent_id, name);
    auto handler = new LookupHandler(shared_from_this(), item_id, ctx);
    return handler->get_future();
}

boost::future<Item> DavProvider::metadata(
    string const& item_id, vector<string> const& metadata_keys,
    Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    auto handler = new MetadataHandler(shared_from_this(), item_id, ctx);
    return handler->get_future();
}

boost::future<Item> DavProvider::create_folder(
    string const& parent_id, string const& name,
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    auto handler = new CreateFolderHandler(
        shared_from_this(), parent_id, name, ctx);
    return handler->get_future();
}

boost::future<unique_ptr<UploadJob>> DavProvider::create_file(
    string const& parent_id, string const& name, int64_t size,
    string const& content_type, bool allow_overwrite,
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    string item_id = make_child_id(parent_id, name);
    boost::promise<unique_ptr<UploadJob>> p;
    p.set_value(unique_ptr<UploadJob>(new DavUploadJob(
        shared_from_this(), item_id, size, content_type, allow_overwrite,
        string(), ctx)));
    return p.get_future();
}

boost::future<unique_ptr<UploadJob>> DavProvider::update(
    string const& item_id, int64_t size, string const& old_etag,
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    boost::promise<unique_ptr<UploadJob>> p;
    p.set_value(unique_ptr<UploadJob>(new DavUploadJob(
        shared_from_this(), item_id, size, string(), true, old_etag, ctx)));
    return p.get_future();
}

boost::future<unique_ptr<DownloadJob>> DavProvider::download(
    string const& item_id, string const& match_etag, Context const& ctx)
{
    boost::promise<unique_ptr<DownloadJob>> p;
    p.set_value(unique_ptr<DownloadJob>(new DavDownloadJob(
        shared_from_this(), item_id, match_etag, ctx)));
    return p.get_future();
}

boost::future<void> DavProvider::delete_item(
    string const& item_id, Context const& ctx)
{
    auto handler = new DeleteHandler(shared_from_this(), item_id, ctx);
    return handler->get_future();
}

boost::future<Item> DavProvider::move(
    string const& item_id, string const& new_parent_id, string const& new_name,
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    auto handler = new CopyMoveHandler(
        shared_from_this(), item_id, new_parent_id, new_name, false, ctx);
    return handler->get_future();
}

boost::future<Item> DavProvider::copy(
    string const& item_id, string const& new_parent_id, string const& new_name,
    vector<string> const& metadata_keys, Context const& ctx)
{
    Q_UNUSED(metadata_keys);
    auto handler = new CopyMoveHandler(
        shared_from_this(), item_id, new_parent_id, new_name, true, ctx);
    return handler->get_future();
}

Item DavProvider::make_item(QUrl const& href, QUrl const& base_url,
                            vector<MultiStatusProperty> const& properties) const
{
    Item item;
    item.item_id = url_to_id(href, base_url);

    QByteArray path = href.path(QUrl::FullyEncoded |
                                QUrl::StripTrailingSlash).toUtf8();
    int pos = path.lastIndexOf('/');
    assert(pos >= 0);
    try
    {
        item.parent_ids.emplace_back(url_to_id(
            href.resolved(QUrl::fromEncoded(path.mid(0, pos+1),
                                            QUrl::StrictMode)), base_url));
    }
    catch (RemoteCommsException const&)
    {
        // Path is outside of base URL: don't expose.
    }

    item.name = QUrl::fromPercentEncoding(path.mid(pos+1)).toStdString();
    item.type = ItemType::file;

    for (const auto& prop : properties)
    {
        if (prop.status != 200)
        {
            // Don't warn about "404 Not Found" properties
            if (prop.status != 404)
            {
                qWarning() << "Got status" << prop.status << "for property"
                           << prop.ns << prop.name;
            }
            continue;
        }
        if (prop.ns == "DAV:")
        {
            if (prop.name == "resourcetype")
            {
                if (prop.value == "DAV:collection") {
                    item.type = ItemType::folder;
                }
            }
            if (prop.name == "getetag")
            {
                item.etag = prop.value.toStdString();
            }
            else if (prop.name == "getcontentlength")
            {
                item.metadata[SIZE_IN_BYTES] = static_cast<int64_t>(prop.value.toLongLong());
            }
            else if (prop.name == "creationdate")
            {
                auto date = QDateTime::fromString(prop.value, Qt::RFC2822Date);
                if (date.isValid())
                {
                    item.metadata[CREATION_TIME] = date.toString(Qt::ISODate).toStdString();
                }
            }
            else if (prop.name == "getlastmodified")
            {
                auto date = QDateTime::fromString(prop.value, Qt::RFC2822Date);
                if (date.isValid())
                {
                    item.metadata[LAST_MODIFIED_TIME] = date.toString(Qt::ISODate).toStdString();
                }
            }
        }
    }

    if (href == base_url)
    {
        item.type = ItemType::root;
        item.name = "Root";
        item.parent_ids.clear();
    }
    return item;
}
