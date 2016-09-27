#pragma once

#include "DavProvider.h"

class OwncloudProvider : public DavProvider
{
public:
    OwncloudProvider();
    virtual ~OwncloudProvider();

    QUrl base_url(
        unity::storage::provider::Context const& ctx) const override;
    QNetworkReply *send_request(
        QNetworkRequest& request, QByteArray const& verb, QIODevice* data,
        unity::storage::provider::Context const& ctx) const override;
};
