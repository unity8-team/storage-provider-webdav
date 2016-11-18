#include "../../src/http_error.h"

#include <gtest/gtest.h>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <unity/storage/provider/Exceptions.h>

using namespace std;
using namespace unity::storage::provider;

class FakeReply : public QNetworkReply
{
public:
    FakeReply(QString const& method, int status, QString const& reason,
              QString const& content_type, QByteArray const& body)
    {
        QNetworkRequest request;
        request.setAttribute(QNetworkRequest::CustomVerbAttribute, method);
        setRequest(request);

        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, reason);

        setHeader(QNetworkRequest::ContentTypeHeader, content_type);
        setHeader(QNetworkRequest::ContentLengthHeader, body.size());
        buffer_.setData(body);
        buffer_.open(QIODevice::ReadOnly);
        open(QIODevice::ReadOnly);
        setFinished(true);
    }

    FakeReply(QNetworkReply::NetworkError code, QString const& message)
    {
        setError(code, message);
        setFinished(true);
    }

    void abort() override
    {
        close();
    }

    qint64 bytesAvailable() const override
    {
        return QNetworkReply::bytesAvailable() + buffer_.bytesAvailable();
    }

    bool isSequential() const override
    {
        return true;
    }

    qint64 size() const override
    {
        return buffer_.size();
    }

    qint64 readData(char *data, qint64 length) override
    {
        return buffer_.read(data, length);
    }

private:
    QBuffer buffer_;
};

template <typename E>
E expect_throw(boost::exception_ptr p)
{
    try
    {
        boost::rethrow_exception(p);
    }
    catch (E const& e)
    {
        return e;
    }
    throw std::runtime_error("No exception thrown");
}


TEST(HttpErrorTests, network_error)
{
    FakeReply reply(QNetworkReply::HostNotFoundError, "Host not found");

    auto e = expect_throw<RemoteCommsException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("Host not found", e.error_message());
}

TEST(HttpErrorTests, text_plain_body)
{
    FakeReply reply("GET", 400, "Bad Request",
                    "text/plain;charset=US-ASCII", "error message");
    auto e = expect_throw<RemoteCommsException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("error message", e.error_message());
}

TEST(HttpErrorTests, application_xml_body)
{
    static const char xml[] = R"(<?xml version="1.0" encoding="utf-8"?>
<d:error xmlns:d='DAV:' xmlns:s='http://sabredav.org/ns'>
  <s:exception>exception_name</s:exception>
  <s:message>error message</s:message>
  <s:sabredav-version>1.8.12</s:sabredav-version>
</d:error>
)";
    FakeReply reply("GET", 400, "Bad Request",
                    "application/xml;charset=utf-8", xml);

    auto e = expect_throw<RemoteCommsException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("exception_name: error message", e.error_message());
}

TEST(HttpErrorTests, fallback_body)
{
    FakeReply reply("GET", 400, "Bad Request",
                    "application/octet-stream", "foo");
    auto e = expect_throw<RemoteCommsException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("Bad Request", e.error_message());
}

TEST(HttpErrorTests, body_provided_separately)
{
    static const char xml[] = R"(<?xml version="1.0" encoding="utf-8"?>
<d:error xmlns:d='DAV:' xmlns:s='http://sabredav.org/ns'>
  <s:exception>exception_name</s:exception>
  <s:message>error message</s:message>
  <s:sabredav-version>1.8.12</s:sabredav-version>
</d:error>
)";
    FakeReply reply("GET", 400, "Bad Request",
                    "application/xml;charset=utf-8", "foo");

    auto e = expect_throw<RemoteCommsException>(
        translate_http_error(&reply, xml));
    EXPECT_EQ("exception_name: error message", e.error_message());
}

TEST(HttpErrorTests, 401)
{
    FakeReply reply("GET", 401, "Unauthorised", "text/plain", "message");

    auto e = expect_throw<PermissionException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("message", e.error_message());
}

TEST(HttpErrorTests, 403)
{
    FakeReply reply("GET", 403, "Forbidden", "text/plain", "message");

    auto e = expect_throw<PermissionException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("message", e.error_message());
}

TEST(HttpErrorTests, 451)
{
    FakeReply reply("GET", 451, "Unavailable for Legal Reasons",
                    "text/plain", "message");

    auto e = expect_throw<PermissionException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("message", e.error_message());
}

TEST(HttpErrorTests, 404)
{
    FakeReply reply("GET", 404, "Not Found", "text/plain", "File not found");

    auto e = expect_throw<NotExistsException>(
        translate_http_error(&reply, QByteArray(), "item_id"));
    EXPECT_EQ("File not found", e.error_message());
    EXPECT_EQ("item_id", e.key());
}

TEST(HttpErrorTests, 410)
{
    FakeReply reply("GET", 410, "Gone", "text/plain", "File not found");

    auto e = expect_throw<NotExistsException>(
        translate_http_error(&reply, QByteArray(), "item_id"));
    EXPECT_EQ("File not found", e.error_message());
    EXPECT_EQ("item_id", e.key());
}

TEST(HttpErrorTests, 405)
{
    // 405 for MKCOL means the target exists
    {
        FakeReply reply("MKCOL", 405, "Method Not Allowed",
                        "text/plain", "message");

        auto e = expect_throw<ExistsException>(
            translate_http_error(&reply, QByteArray(), "item_id"));
        EXPECT_EQ("message", e.error_message());
        EXPECT_EQ("item_id", e.native_identity());
    }
    // Otherwise, it is unknown
    {
        FakeReply reply("GET", 405, "Method Not Allowed",
                        "text/plain", "message");

        auto e = expect_throw<UnknownException>(
            translate_http_error(&reply, QByteArray(), "item_id"));
        EXPECT_EQ("HTTP 405: message", e.error_message());
    }
}

TEST(HttpErrorTests, 409)
{
    FakeReply reply("GET", 409, "Conflict", "text/plain", "message");

    auto e = expect_throw<ConflictException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("message", e.error_message());
}

TEST(HttpErrorTests, 412)
{
    FakeReply reply("GET", 412, "Precondition failed", "text/plain", "message");

    auto e = expect_throw<ConflictException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("message", e.error_message());
}

TEST(HttpErrorTests, 507)
{
    FakeReply reply("GET", 507, "Insufficient Storage",
                    "text/plain", "message");

    auto e = expect_throw<QuotaException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("message", e.error_message());
}

TEST(HttpErrorTests, other)
{
    FakeReply reply("GET", 500, "Internal Server Error",
                    "text/plain", "message");

    auto e = expect_throw<UnknownException>(
        translate_http_error(&reply, QByteArray()));
    EXPECT_EQ("HTTP 500: message", e.error_message());
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
