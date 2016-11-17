#include "http_error.h"

#include <QNetworkReply>
#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <unity/storage/provider/Exceptions.h>

#include <cassert>

using namespace std;
using namespace unity::storage::provider;

namespace {

class ErrorParser : public QXmlDefaultHandler
{
public:
    bool startElement(QString const& namespace_uri, QString const& local_name,
                      QString const& qname, QXmlAttributes const& attrs) override;
    bool endElement(QString const& namespace_uri, QString const& local_name,
                    QString const& qname) override;
    bool characters(QString const& data) override;

    string exception_;
    string message_;

private:
    static QString const SABREDAV_NS;

    QString char_data_;
};

QString const ErrorParser::SABREDAV_NS("http://sabredav.org/ns");

bool ErrorParser::startElement(QString const& namespace_uri,
                               QString const& local_name,
                               QString const& qname,
                               QXmlAttributes const& attrs)
{
    Q_UNUSED(namespace_uri);
    Q_UNUSED(local_name);
    Q_UNUSED(qname);
    Q_UNUSED(attrs);

    char_data_.clear();
    return true;
}

bool ErrorParser::endElement(QString const& namespace_uri,
                             QString const& local_name,
                             QString const& qname)
{
    Q_UNUSED(qname);
    if (namespace_uri == SABREDAV_NS)
    {
        if (local_name == "exception")
        {
            exception_ = char_data_.toStdString();
        }
        else if (local_name == "message")
        {
            message_ = char_data_.toStdString();
        }
    }
    char_data_.clear();
    return true;
}

bool ErrorParser::characters(QString const& data)
{
    char_data_ += data;
    return true;
}

std::string make_message(QNetworkReply *reply,
                         QByteArray body)
{
    if (body.isEmpty())
    {
        body = reply->readAll();
    }
    auto content_type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    int semicolon = content_type.indexOf(';');
    if (semicolon >= 0)
    {
        content_type = content_type.left(semicolon);
    }
    if (content_type == "text/plain")
    {
        return body.toStdString();
    }
    else if (content_type == "application/xml")
    {
        ErrorParser parser;
        QXmlSimpleReader reader;
        QXmlInputSource source;
        reader.setContentHandler(&parser);
        source.setData(body);
        if (reader.parse(source)) {
            return parser.exception_ + ": " + parser.message_;
        }
    }
    // Fall back to returning the HTTP status string
    return reply->attribute(
        QNetworkRequest::HttpReasonPhraseAttribute).toString().toStdString();
}

}

boost::exception_ptr translate_http_error(QNetworkReply *reply,
                                          QByteArray const& body,
                                          string const& item_id)
{
    assert(reply != nullptr);
    assert(reply->isFinished());

    auto status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 0)
    {
        return boost::copy_exception(
            RemoteCommsException(reply->errorString().toStdString()));
    }

    auto message = make_message(reply, body);

    switch (status)
    {
    case 400: // Bad Request
        return boost::copy_exception(RemoteCommsException(message));

    case 401: // Unauthorised
        // This should be separate from Forbidden, triggering reauth.
    case 403: // Forbidden
    case 451: // Unavailable for Legal Reasons
        return boost::copy_exception(PermissionException(message));

    case 404: // Not found
    case 410: // Gone
        return boost::copy_exception(NotExistsException(message, item_id));

    case 409: // Conflict
    case 412: // Precondition failed
        return boost::copy_exception(ConflictException(message));

    case 507: // Insufficient Storage
        return boost::copy_exception(QuotaException(message));
    }
    return boost::copy_exception(
        UnknownException("HTTP " + to_string(status) + ": " + message));
}
