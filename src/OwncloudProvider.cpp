#include "OwncloudProvider.h"

using namespace std;
using namespace unity::storage::provider;

OwncloudProvider::OwncloudProvider()
{
}

OwncloudProvider::~OwncloudProvider() = default;

string OwncloudProvider::base_url(Context const& ctx) const
{
    return "http://whatever.com/";
}

void OwncloudProvider::add_credentials(QNetworkRequest *request,
                                       Context const& ctx) const
{
}
