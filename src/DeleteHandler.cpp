#include "DeleteHandler.h"

#include <unity/storage/provider/Exceptions.h>

#include "DavProvider.h"
#include "item_id.h"

using namespace std;
using namespace unity::storage::provider;

DeleteHandler::DeleteHandler(DavProvider const& provider,
                             string const& item_id,
                             Context const& ctx)
    : provider_(provider), item_id_(item_id)
{
    QUrl const base_url = provider.base_url(ctx);
    QNetworkRequest request(id_to_url(item_id_, base_url));
    reply_.reset(provider.send_request(request, QByteArrayLiteral("DELETE"),
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
        promise_.set_exception(RemoteCommsException("Error from DELETE: " + to_string(status)));
    }

    deleteLater();
}
