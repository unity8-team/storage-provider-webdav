#pragma once

#include <QBuffer>
#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/Exceptions.h>

#include <memory>

#include "DavProvider.h"
#include "MultiStatusParser.h"

class PropFindHandler : public QObject {
    Q_OBJECT
public:
    PropFindHandler(DavProvider const& provider,
                    std::string const& item_id, int depth,
                    unity::storage::provider::Context const& ctx);
    ~PropFindHandler();

    void abort();

private Q_SLOTS:
    // From QNetworkReply
    void onReplyReadyRead();
    void onReplyFinished();

    // From MultiStatusParser
    void onParserResponse(QUrl const& href, std::vector<MultiStatusProperty> const& properties, int status);
    void onParserFinished();

private:
    void reportError(unity::storage::provider::StorageException const& error);
    void reportError(boost::exception_ptr const& ep);
    void reportSuccess();

    bool seen_headers_ = false;
    bool is_error_ = false;
    bool finished_ = false;
    boost::promise<unity::storage::provider::ItemList> promise_;

    DavProvider const& provider_;
    QUrl base_url_;
    QBuffer request_body_;
    std::unique_ptr<QNetworkReply> reply_;
    std::unique_ptr<MultiStatusParser> parser_;
    QByteArray error_body_;

protected:
    virtual void finish() = 0;

    unity::storage::provider::ItemList items_;
    boost::exception_ptr error_;
};
