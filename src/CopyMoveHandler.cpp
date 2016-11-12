#include "CopyMoveHandler.h"
#include "RetrieveMetadataHandler.h"
#include "item_id.h"

using namespace std;
using namespace unity::storage::provider;

CopyMoveHandler::CopyMoveHandler(DavProvider const& provider,
                                 string const& item_id,
                                 string const& new_parent_id,
                                 string const& new_name,
                                 bool copy,
                                 Context const& ctx)
    : provider_(provider), item_id_(item_id),
      new_item_id_(make_child_id(new_parent_id, new_name, is_folder(item_id))),
      context_(ctx)
{
    QUrl const base_url = provider.base_url(ctx);
    QNetworkRequest request(id_to_url(item_id_, base_url));
    request.setRawHeader(QByteArrayLiteral("Destination"),
                         id_to_url(new_item_id_, base_url).toEncoded());
    // FIXME: what behaviour do we expect when the destination exists?
    request.setRawHeader(QByteArrayLiteral("Overwrite"),
                         QByteArrayLiteral("F"));

    reply_.reset(provider.send_request(
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
        promise_.set_exception(RemoteCommsException("Error from request: " + to_string(status)));
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
