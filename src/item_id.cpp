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
string make_child_id(string const& parent_id, string const& name)
{
    string item_id = parent_id;
    if (item_id == ".")
    {
        item_id.clear();
    }
    if (item_id.size() == 0 || item_id[item_id.size()-1] != '/')
    {
        item_id += '/';
    }
    item_id += QUrl::toPercentEncoding(QString::fromStdString(name)).toStdString();
    return item_id;
}
