#pragma once

#include <QBuffer>
#include <QObject>
#include <QNetworkReply>
#include <unity/storage/provider/Exceptions.h>

#include <memory>

#include "DavProvider.h"
#include "MultiStatusParser.h"

class RootsHandler : public QObject {
    Q_OBJECT
public:
    RootsHandler(DavProvider const& provider,
                 unity::storage::provider::Context const& ctx);
    ~RootsHandler();

    boost::future<unity::storage::provider::ItemList> get_future();

private:
    void sendError(unity::storage::provider::StorageException const& error);
    void sendResponse();

private Q_SLOTS:
    // From QNetworkReply
    void onError(QNetworkReply::NetworkError code);
    void onReadyRead();

    // From MultiStatusParser
    void onParserResponse(QUrl const& href, std::vector<MultiStatusProperty> const& properties, int status);
    void onParserFinished();

private:
    bool promise_set_ = false;
    boost::promise<unity::storage::provider::ItemList> promise_;

    DavProvider const& provider_;
    QUrl base_url_;
    QBuffer request_body_;
    std::unique_ptr<QNetworkReply> reply_;

    std::unique_ptr<MultiStatusParser> parser_;
    unity::storage::provider::ItemList roots_;
};
