#include "../../src/MultiStatusParser.h"

#include <gtest/gtest.h>
#include <QBuffer>
#include <QCoreApplication>
#include <QSignalSpy>

TEST(MultiStatus, xxx) {
    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);
    buffer.write("<a><b><c/></b></a>");
    buffer.seek(0);

    MultiStatusParser parser(&buffer);
    QSignalSpy spy(&parser, &MultiStatusParser::finished);

    parser.startParsing();
}


int main(int argc, char**argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
