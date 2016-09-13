#include "DavProvider.h"

using namespace std;
using namespace unity::storage::provider;

DavProvider::DavProvider()
{
}

DavProvider::~DavProvider() = default;

boost::future<ItemList> DavProvider::roots(Context const& ctx)
{
}

boost::future<tuple<ItemList,string>> DavProvider::list(
    string const& item_id, string const& page_token, Context const& ctx)
{
}

boost::future<ItemList> DavProvider::lookup(
    string const& parent_id, string const& name, Context const& ctx)
{
}

boost::future<Item> DavProvider::metadata(
    string const& item_id, Context const& ctx)
{
}

boost::future<Item> DavProvider::create_folder(
    string const& parent_id, string const& name, Context const& ctx)
{
}

boost::future<unique_ptr<UploadJob>> DavProvider::create_file(
    string const& parent_id, string const& name, int64_t size,
    string const& content_type, bool allow_overwrite, Context const& ctx)
{
}

boost::future<unique_ptr<UploadJob>> DavProvider::update(
    string const& item_id, int64_t size, string const& old_etag,
    Context const& ctx)
{
}

boost::future<unique_ptr<DownloadJob>> DavProvider::download(
    string const& item_id, Context const& ctx)
{
}

boost::future<void> DavProvider::delete_item(
    string const& item_id, Context const& ctx)
{
}

boost::future<Item> DavProvider::move(
    string const& item_id, string const& new_parent_id, string const& new_name,
    Context const& ctx)
{
}

boost::future<Item> DavProvider::copy(
    string const& item_id, string const& new_parent_id, string const& new_name,
    Context const& ctx)
{
}
