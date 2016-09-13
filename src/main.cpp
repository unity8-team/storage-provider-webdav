#include <unity/storage/provider/Server.h>

#include "OwncloudProvider.h"

using namespace std;
using namespace unity::storage::provider;

int main(int argc, char **argv)
{
    string const bus_name = "com.canonical.StorageFramework.Provider.OwnCloud";
    string const account_service_id = "storage-provider-owncloud";

    Server<OwncloudProvider> server(bus_name, account_service_id);
    server.init(argc, argv);
    server.run();
    return 0;
}
