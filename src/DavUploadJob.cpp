#include "DavUploadJob.h"
#include "DavProvider.h"
#include "item_id.h"

#include <unity/storage/provider/Exceptions.h>

#include <cassert>
#include <cstdint>
#include <string>

using namespace std;
using namespace unity::storage::provider;

namespace
{

string make_upload_id()
{
    static int counter = 0;
    return to_string(counter++);
}

}

DavUploadJob::DavUploadJob(DavProvider const& provider, string const& item_id,
                           int64_t size, string const& old_etag,
                           Context const& ctx)
    : QObject(), UploadJob(make_upload_id()), provider_(provider),
      base_url_(provider.base_url(ctx)), size_(size)
{
    QNetworkRequest request(id_to_url(item_id, base_url_));
    if (!old_etag.empty())
    {
        request.setRawHeader(QByteArrayLiteral("If-Match"),
                             QByteArray::fromStdString(old_etag));
    }
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant::fromValue(size));
    request.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, true);

    reader_.setSocketDescriptor(
        read_socket(), QLocalSocket::ConnectedState, QIODevice::ReadOnly);
    reply_.reset(provider.send_request(
        request, QByteArrayLiteral("PUT"), &reader_, ctx));
    assert(reply_.get() != nullptr);
}

DavUploadJob::~DavUploadJob() = default;

boost::future<void> DavUploadJob::cancel()
{
    return boost::make_ready_future();
}

boost::future<Item> DavUploadJob::finish()
{
    return boost::make_exceptional_future<Item>(RemoteCommsException("whatever"));
}
