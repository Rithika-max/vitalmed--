#include "server/HttpServer.hpp"
#include <drogon/drogon.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace vetalmed::server {

static bool isPortAvailable(int port) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
#endif
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    bool available = true;
    if (bind(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
        available = false;
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    return available;
}

void HttpServer::start(int port) {
    if (!isPortAvailable(port)) {
        throw std::runtime_error("Port " + std::to_string(port) + " is already in use. Please free the port or use a different port.");
    }
    drogon::app().addListener("0.0.0.0", port);
}

}
