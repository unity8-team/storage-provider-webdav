#include "DavEnvironment.h"
#include <testsetup.h>

#include <QDebug>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <chrono>
#include <stdexcept>
#include <thread>

using namespace std;

namespace
{

// Get a probably free IPv4 port number.  We do this by binding a
// socket and then closing it.  It is possible that the port will be
// reused, but that is less likely than with fixed port numbers.
int get_free_port()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        throw runtime_error("get_free_port: Could not create socket");
    }

    struct sockaddr_in addr;
    socklen_t length = sizeof(addr);
    memset(&addr, 0, length);
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), length) < 0)
    {
        close(sock);
        throw runtime_error("get_free_port: Could not bind socket");
    }

    if (getsockname(sock, reinterpret_cast<struct sockaddr*>(&addr),
                    &length) < 0 || length > sizeof(addr))
    {
        close(sock);
        throw runtime_error("get_free_port: Could not get socket name");
    }
    close(sock);
    return ntohs(addr.sin_port);
}

void wait_for_listen(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        throw runtime_error("wait_for_listen: Could not create socket");
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton("127.0.0.1", &addr.sin_addr);

    bool success = false;
    for (int i = 0; i < 100; i++)
    {
        if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr),
                    sizeof(addr)) == 0)
        {
            success = true;
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    close(sock);
    if (!success)
    {
        throw runtime_error("wait_for_listen: Could not connect to socket");
    }
}

}

DavEnvironment::DavEnvironment(QString const& base_dir)
{
    int port = get_free_port();

    server_.setProcessChannelMode(QProcess::ForwardedChannels);
    server_.setWorkingDirectory(base_dir);
    server_.start("php", {"-S", QStringLiteral("127.0.0.1:%1").arg(port),
                          TEST_SRC_DIR "/utils/sabredav-server.php"});
    if (!server_.waitForStarted())
    {
        throw runtime_error("DavServer::DavServer(): wait for server start failed");
    }
    wait_for_listen(port);

    base_url_.setUrl(QStringLiteral("http://127.0.0.1:%1/").arg(port),
                     QUrl::StrictMode);
}

DavEnvironment::~DavEnvironment()
{
    server_.terminate();
    if (!server_.waitForFinished())
    {
        qCritical() << "Failed to terminate DAV server";
    }
}

QUrl const& DavEnvironment::base_url() const
{
    return base_url_;
}
