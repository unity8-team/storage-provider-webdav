#include "DavEnvironment.h"
#include <testsetup.h>

#include <QDebug>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdexcept>

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
    socklen_t length = sizeof(struct sockaddr_in);
    memset(&addr, 0, length);
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), length) < 0)
    {
        close(sock);
        throw runtime_error("get_free_port: Could not bind socket");
    }

    if (getsockname(sock, reinterpret_cast<struct sockaddr*>(&addr),
                    &length) < 0 || length > sizeof(struct sockaddr_in))
    {
        close(sock);
        throw runtime_error("get_free_port: Could not get socket name");
    }
    close(sock);
    return ntohs(addr.sin_port);
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
