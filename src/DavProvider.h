#pragma once

#include <unity/storage/provider/ProviderBase.h>

class DavProvider : public unity::storage::provider::ProviderBase
{
public:
    DavProvider();
    virtual ~DavProvider();

    boost::future<unity::storage::provider::ItemList> roots(
        unity::storage::provider::Context const& ctx) override;
    boost::future<std::tuple<unity::storage::provider::ItemList,std::string>> list(
        std::string const& item_id, std::string const& page_token,
        unity::storage::provider::Context const& ctx) override;
    boost::future<unity::storage::provider::ItemList> lookup(
        std::string const& parent_id, std::string const& name,
        unity::storage::provider::Context const& ctx) override;
    boost::future<unity::storage::provider::Item> metadata(
        std::string const& item_id,
        unity::storage::provider::Context const& ctx) override;

    boost::future<unity::storage::provider::Item> create_folder(
        std::string const& parent_id, std::string const& name,
        unity::storage::provider::Context const& ctx) override;
    boost::future<std::unique_ptr<unity::storage::provider::UploadJob>> create_file(
        std::string const& parent_id, std::string const& name,
        int64_t size, std::string const& content_type, bool allow_overwrite,
        unity::storage::provider::Context const& ctx) override;
    boost::future<std::unique_ptr<unity::storage::provider::UploadJob>> update(
        std::string const& item_id, int64_t size,
        std::string const& old_etag,
        unity::storage::provider::Context const& ctx) override;

    boost::future<std::unique_ptr<unity::storage::provider::DownloadJob>> download(
        std::string const& item_id,
        unity::storage::provider::Context const& ctx) override;

    boost::future<void> delete_item(std::string const& item_id,
        unity::storage::provider::Context const& ctx) override;
    boost::future<unity::storage::provider::Item> move(
        std::string const& item_id, std::string const& new_parent_id,
        std::string const& new_name,
        unity::storage::provider::Context const& ctx) override;
    boost::future<unity::storage::provider::Item> copy(
        std::string const& item_id, std::string const& new_parent_id,
        std::string const& new_name,
        unity::storage::provider::Context const& ctx) override;
};
