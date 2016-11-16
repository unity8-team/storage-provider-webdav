#include "CreateFolderHandler.h"
#include "RetrieveMetadataHandler.h"
#include "item_id.h"

using namespace std;
using namespace unity::storage::provider;

CreateFolderHandler::CreateFolderHandler(DavProvider const& provider,
                                         string const& parent_id,
                                         string const& name,
                                         Context const& ctx)
    : provider_(provider), item_id_(make_child_id(parent_id, name, true)),
      context_(ctx)
{
    QUrl const base_url = provider.base_url(ctx);
    QNetworkRequest request(id_to_url(item_id_, base_url));
    mkcol_.reset(provider.send_request(request, QByteArrayLiteral("MKCOL"),
                                       nullptr, ctx));
    connect(mkcol_.get(), &QNetworkReply::finished,
            this, &CreateFolderHandler::onFinished);
}

CreateFolderHandler::~CreateFolderHandler() = default;

boost::future<Item> CreateFolderHandler::get_future()
{
    return promise_.get_future();
}

void CreateFolderHandler::onFinished()
{
    auto status = mkcol_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (status != 201)
    {
        promise_.set_exception(RemoteCommsException("Error from MKCOL: " + to_string(status)));
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
