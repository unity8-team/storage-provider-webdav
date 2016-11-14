#include "http_error.h"

#include <QNetworkReply>
#include <unity/storage/provider/Exceptions.h>

using namespace std;
using namespace unity::storage::provider;

boost::exception_ptr translate_http_error(QNetworkReply *reply,
                                          string const& item_id)
{
    auto status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    auto reason = reply->attribute(
        QNetworkRequest::HttpReasonPhraseAttribute).toString().toStdString();

    if (status == 0)
    {
        return boost::copy_exception(
            RemoteCommsException(reply->errorString().toStdString()));
    }
    switch (status)
    {
    case 400: // Bad Request
        return boost::copy_exception(RemoteCommsException(reason));

    case 401: // Unauthorised
        // This should be separate from Forbidden, triggering reauth.
    case 403: // Forbidden
    case 451: // Unavailable for Legal Reasons
        return boost::copy_exception(PermissionException(reason));

    case 404: // Not found
    case 410: // Gone
        return boost::copy_exception(
            NotExistsException(reason, item_id));

    case 409: // Conflict
    case 412: // Precondition failed
        return boost::copy_exception(
            ConflictException(reason));

    case 507: // Insufficient Storage
        return boost::copy_exception(
            QuotaException(reason));
    }
    return boost::copy_exception(
        RemoteCommsException(to_string(status) + " " + reason));
}
