#include "../../src/MultiStatusParser.h"

#include <gtest/gtest.h>
#include <QBuffer>
#include <QCoreApplication>
#include <QSignalSpy>

using namespace std;

TEST(MultiStatus, garbage_input)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    buffer.write("garbage 897123987123987123987132");
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy response_spy(&parser, &MultiStatusParser::response);
    QSignalSpy finished_spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
    Q_EMIT buffer.readChannelFinished();
    ASSERT_EQ(1, finished_spy.count());
    EXPECT_EQ("error occurred while parsing element", parser.errorString()) << parser.errorString().toStdString();
    EXPECT_EQ(0, response_spy.count());
}

TEST(MultiStatus, invalid_xml)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    buffer.write(R"(
<D:multistatus xmlns:D='DAV:'>
  <D:response>
    <D:href>foo
  </D:response>
</D:multistatus>
)");
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy response_spy(&parser, &MultiStatusParser::response);
    QSignalSpy finished_spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
    Q_EMIT buffer.readChannelFinished();
    ASSERT_EQ(1, finished_spy.count());
    EXPECT_EQ("tag mismatch", parser.errorString()) << parser.errorString().toStdString();
    EXPECT_EQ(0, response_spy.count());
}

TEST(MultiStatus, incomplete_xml)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    buffer.write(R"(
<D:multistatus xmlns:D='DAV:'>
  <D:response>
    <D:href>foo</D:href>
  </D:response>
)");
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy response_spy(&parser, &MultiStatusParser::response);
    QSignalSpy finished_spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
    Q_EMIT buffer.readChannelFinished();
    ASSERT_EQ(1, finished_spy.count());
    EXPECT_EQ("unexpected end of file", parser.errorString()) << parser.errorString().toStdString();
}

TEST(MultiStatus, non_multistatus_xml)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    buffer.write("<a><b><c/></b></a>");
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy response_spy(&parser, &MultiStatusParser::response);
    QSignalSpy finished_spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
    Q_EMIT buffer.readChannelFinished();
    ASSERT_EQ(1, finished_spy.count());
    EXPECT_EQ("", parser.errorString()) << parser.errorString().toStdString();
    EXPECT_EQ(0, response_spy.count());
}

TEST(MultiStatus, response_status)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    // Multi-status response taken from RFC 4918 Section 9.10.9
    buffer.write(R"(<?xml version="1.0" encoding="utf-8" ?>
<D:multistatus xmlns:D="DAV:">
  <D:response>
    <D:href>http://example.com/webdav/secret</D:href>
    <D:status>HTTP/1.1 403 Forbidden</D:status>
  </D:response>
  <D:response>
    <D:href>http://example.com/webdav/</D:href>
    <D:status>HTTP/1.1 424 Failed Dependency</D:status>
  </D:response>
</D:multistatus>
)");
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy response_spy(&parser, &MultiStatusParser::response);
    QSignalSpy finished_spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
    Q_EMIT buffer.readChannelFinished();
    ASSERT_EQ(1, finished_spy.count());
    EXPECT_EQ("", parser.errorString()) << parser.errorString().toStdString();

    ASSERT_EQ(2, response_spy.count());
    QList<QVariant> args = response_spy.takeFirst();
    EXPECT_EQ("http://example.com/webdav/secret", args[0].value<QString>());
    EXPECT_EQ(0, args[1].value<vector<MultiStatusProperty>>().size());
    EXPECT_EQ("HTTP/1.1 403 Forbidden", args[2].value<QString>());

    args = response_spy.takeFirst();
    EXPECT_EQ("http://example.com/webdav/", args[0].value<QString>());
    EXPECT_EQ(0, args[1].value<vector<MultiStatusProperty>>().size());
    EXPECT_EQ("HTTP/1.1 424 Failed Dependency", args[2].value<QString>());
}

TEST(MultiStatus, response_properties)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    // Multi-status response based on one from from RFC 4918 Section 9.1.3
    buffer.write(R"(<?xml version="1.0" encoding="utf-8" ?>
<D:multistatus xmlns:D="DAV:">
  <D:response xmlns:R='http://ns.example.com/boxschema/'>
    <D:href>http://www.example.com/file</D:href>
    <D:propstat>
      <D:prop>
        <R:bigbox>
          Box type A
        </R:bigbox>
        <R:author>
          <R:Name>J.J. Johnson</R:Name>
        </R:author>
      </D:prop>
      <D:status>HTTP/1.1 200 OK</D:status>
    </D:propstat>
    <D:propstat>
      <D:prop><R:DingALing/><R:Random/></D:prop>
      <D:status>HTTP/1.1 403 Forbidden</D:status>
      <D:responsedescription> The user does not have access to the DingALing property.
      </D:responsedescription>
    </D:propstat>
  </D:response>
  <D:responsedescription> There has been an access violation error.
  </D:responsedescription>
</D:multistatus>
        )");
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy response_spy(&parser, &MultiStatusParser::response);
    QSignalSpy finished_spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
    Q_EMIT buffer.readChannelFinished();
    ASSERT_EQ(1, finished_spy.count());
    EXPECT_EQ("", parser.errorString()) << parser.errorString().toStdString();

    ASSERT_EQ(1, response_spy.count());
    QList<QVariant> args = response_spy.takeFirst();
    EXPECT_EQ("http://www.example.com/file", args[0].value<QString>());
    EXPECT_EQ("", args[2].value<QString>());
    auto props = args[1].value<vector<MultiStatusProperty>>();

    ASSERT_EQ(4, props.size());

    EXPECT_EQ("http://ns.example.com/boxschema/", props[0].ns);
    EXPECT_EQ("bigbox", props[0].name);
    EXPECT_EQ("Box type A", props[0].value);
    EXPECT_EQ("HTTP/1.1 200 OK", props[0].status);

    EXPECT_EQ("http://ns.example.com/boxschema/", props[1].ns);
    EXPECT_EQ("author", props[1].name);
    // TODO: handle structured properties.
    EXPECT_EQ("", props[1].value);
    EXPECT_EQ("HTTP/1.1 200 OK", props[1].status);

    EXPECT_EQ("http://ns.example.com/boxschema/", props[2].ns);
    EXPECT_EQ("DingALing", props[2].name);
    EXPECT_EQ("", props[2].value);
    EXPECT_EQ("HTTP/1.1 403 Forbidden", props[2].status);

    EXPECT_EQ("http://ns.example.com/boxschema/", props[3].ns);
    EXPECT_EQ("Random", props[3].name);
    EXPECT_EQ("", props[3].value);
    EXPECT_EQ("HTTP/1.1 403 Forbidden", props[3].status);
}

TEST(MultiStatus, streaming_parse)
{
    static char const first_chunk[] = R"(
     <D:multistatus xmlns:D='DAV:'>
       <D:response>
         <D:href>/container/</D:href>
         <D:propstat>
           <D:prop>
             <D:creationdate>1997-12-01T17:42:21-08:00</D:creationdate>
             <D:displayname>Example collection</D:displayname>
             <D:resourcetype><D:collection/></D:resourcetype>
           </D:prop>
           <D:status>HTTP/1.1 200 OK</D:status>
         </D:propstat>
       </D:response>
       <D:response>
         <D:href>/container/front.html</D:href>
         <D:propstat>
)";
    static char const second_chunk[] = R"(
           <D:prop>
             <D:creationdate>1997-12-01T18:27:21-08:00</D:creationdate>
             <D:displayname>Example HTML resource</D:displayname>
             <D:getcontentlength>4525</D:getcontentlength>
             <D:getcontenttype>text/html</D:getcontenttype>
             <D:getetag>"zzyzx"</D:getetag>
             <D:getlastmodified
               >Mon, 12 Jan 1998 09:25:56 GMT</D:getlastmodified>
             <D:resourcetype/>
           </D:prop>
           <D:status>HTTP/1.1 200 OK</D:status>
         </D:propstat>
       </D:response>
     </D:multistatus>
)";
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    buffer.write(first_chunk);
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy response_spy(&parser, &MultiStatusParser::response);
    QSignalSpy finished_spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
    ASSERT_EQ(0, finished_spy.count());

    ASSERT_EQ(1, response_spy.count());
    QList<QVariant> args = response_spy.takeFirst();
    EXPECT_EQ("/container/", args[0].value<QString>());
    auto props = args[1].value<vector<MultiStatusProperty>>();
    ASSERT_EQ(3, props.size());
    EXPECT_EQ("creationdate", props[0].name);
    EXPECT_EQ("displayname", props[1].name);
    EXPECT_EQ("resourcetype", props[2].name);

    auto pos = buffer.pos();
    buffer.write(second_chunk);
    buffer.seek(pos);
    Q_EMIT buffer.readyRead();
    Q_EMIT buffer.readChannelFinished();
    ASSERT_EQ(1, finished_spy.count());
    EXPECT_EQ("", parser.errorString()) << parser.errorString().toStdString();

    ASSERT_EQ(1, response_spy.count());
    args = response_spy.takeFirst();
    EXPECT_EQ("/container/front.html", args[0].value<QString>());
    props = args[1].value<vector<MultiStatusProperty>>();
    ASSERT_EQ(7, props.size());
    EXPECT_EQ("creationdate", props[0].name);
    EXPECT_EQ("displayname", props[1].name);
    EXPECT_EQ("getcontentlength", props[2].name);
    EXPECT_EQ("getcontenttype", props[3].name);
    EXPECT_EQ("getetag", props[4].name);
    EXPECT_EQ("getlastmodified", props[5].name);
    EXPECT_EQ("resourcetype", props[6].name);
}

int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
