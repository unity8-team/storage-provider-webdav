#pragma once

#include "DavProvider.h"

class OwncloudProvider : public DavProvider
{
public:
    OwncloudProvider();
    virtual ~OwncloudProvider();

protected:
    QUrl base_url(
        unity::storage::provider::Context const& ctx) const override;
    void add_credentials(
        QNetworkRequest *request,
        unity::storage::provider::Context const& ctx) const override;
};
