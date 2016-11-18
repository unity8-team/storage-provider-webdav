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

#include "item_id.h"

#include <unity/storage/provider/Exceptions.h>

using namespace std;
using namespace unity::storage::provider;

static const auto URL_FORMAT = QUrl::FullyEncoded | QUrl::NormalizePathSegments;

QUrl id_to_url(string const& item_id, QUrl const& base_url)
{
    QUrl item_url(QString::fromStdString(item_id), QUrl::StrictMode);
    if (!item_url.isValid())
    {
        throw InvalidArgumentException("Invalid item ID: " + item_id);
    }

    item_url = base_url.resolved(item_url);

    QByteArray base_url_bytes = base_url.toEncoded(URL_FORMAT);
    QByteArray item_url_bytes = item_url.toEncoded(URL_FORMAT);
    if (!item_url_bytes.startsWith(base_url_bytes))
    {
        throw InvalidArgumentException("Invalid item ID: " + item_id);
    }

    return item_url;
}

string url_to_id(QUrl const& item_url, QUrl const& base_url)
{
    QByteArray base_url_bytes = base_url.toEncoded(URL_FORMAT);
    QByteArray item_url_bytes = item_url.toEncoded(URL_FORMAT);
    if (!item_url_bytes.startsWith(base_url_bytes))
    {
        throw RemoteCommsException("Url is outside of base URL: " + item_url_bytes.toStdString());
    }

    string item_id(item_url_bytes.begin() + base_url_bytes.size(),
                   item_url_bytes.end());
    if (item_id.empty())
    {
        item_id = ".";
    }
    return item_id;
}

// Construct a possible ID for a named child of a parent ID
string make_child_id(string const& parent_id, string const& name, bool is_folder)
{
    if (name == "." || name == "..")
    {
        throw InvalidArgumentException("Invalid name: " + name);
    }

    string item_id = parent_id;
    if (item_id == ".")
    {
        item_id.clear();
    }
    else if (item_id.size() == 0 || item_id[item_id.size()-1] != '/')
    {
        item_id += '/';
    }
    item_id += QUrl::toPercentEncoding(QString::fromStdString(name)).toStdString();

    if (is_folder)
    {
        item_id += '/';
    }

    return item_id;
}

bool is_folder(string const& item_id)
{
    auto size = item_id.size();
    if (size == 0)
    {
        throw InvalidArgumentException("Invalid blank item ID");
    }
    if (size == 1 && item_id[0] == '.')
    {
        return true;
    }
    return item_id[size-1] == '/';
}
