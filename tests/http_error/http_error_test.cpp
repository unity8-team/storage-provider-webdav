#include "../../src/http_error.h"

#include <gtest/gtest.h>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QNetworkReply>
#include <unity/storage/provider/Exceptions.h>

using namespace std;
using namespace unity::storage::provider;

class FakeReply : public QNetworkReply
{
public:
    FakeReply(int status, QString const& reason,
              QString const& content_type, QByteArray const& body)
    {
        setFinished(true);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, reason);

        setHeader(QNetworkRequest::ContentTypeHeader, content_type);
        setHeader(QNetworkRequest::ContentLengthHeader, body.size());
        buffer_.setData(body);
        buffer_.open(QIODevice::ReadOnly);
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

TEST(HttpErrorTests, translate_http_error_404)
{
    FakeReply reply(404, "Not Found", "text/plain", "");

    boost::exception_ptr p = translate_http_error(&reply, QByteArray());
    try
    {
        boost::rethrow_exception(p);
        FAIL();
    }
    catch (NotExistsException const& e)
    {
        EXPECT_EQ("Not Found", e.error_message());
    }
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
