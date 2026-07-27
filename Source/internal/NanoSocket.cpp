/* Copyright (c) 2026, Christian Ahrens
 *
 * This file is part of NanoOcp <https://github.com/ChristianAhrens/NanoOcp>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "NanoSocket.h"

#include <cstring>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
  // Windows – Winsock2 already included via header
  #pragma comment(lib, "Ws2_32.lib")
  #define NANOSOCK_CLOSE(fd)  ::closesocket(fd)
  #define NANOSOCK_ERRNO      WSAGetLastError()
  #define NANOSOCK_WOULDBLOCK WSAEWOULDBLOCK
  #define NANOSOCK_INPROGRESS WSAEWOULDBLOCK
  // Windows select() ignores the nfds argument entirely; pass 0 to avoid
  // UINT_PTR → int truncation warnings on 64-bit builds.
  #define NANOSOCK_NFDS(fd)   0
#else
  // POSIX (macOS, iOS, Linux, …)
  #include <arpa/inet.h>
  #include <errno.h>
  #include <fcntl.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <sys/select.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <unistd.h>
  #define NANOSOCK_CLOSE(fd)  ::close(fd)
  #define NANOSOCK_ERRNO      errno
  #define NANOSOCK_WOULDBLOCK EAGAIN
  #define NANOSOCK_INPROGRESS EINPROGRESS
  #define NANOSOCK_NFDS(fd)   (static_cast<int>(fd) + 1)
#endif

namespace NanoOcp1
{

// ── Platform init ─────────────────────────────────────────────────────────────

void NanoSocket::platformInit()
{
#if defined(_WIN32) || defined(_WIN64)
    static bool initialised = false;
    if (!initialised)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialised = true;
    }
#endif
}

// ── Construction / destruction ────────────────────────────────────────────────

NanoSocket::NanoSocket()
{
    platformInit();
}

NanoSocket::~NanoSocket()
{
    close();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

bool NanoSocket::setNonBlocking(bool nonBlocking)
{
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = nonBlocking ? 1u : 0u;
    return ioctlsocket(m_fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags < 0) return false;
    if (nonBlocking) flags |=  O_NONBLOCK;
    else             flags &= ~O_NONBLOCK;
    return fcntl(m_fd, F_SETFL, flags) == 0;
#endif
}

// ── Client ────────────────────────────────────────────────────────────────────

bool NanoSocket::connect(const std::string& hostName, int portNumber, int timeoutMs)
{
    close();

    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const std::string portStr = std::to_string(portNumber);
    struct addrinfo* res = nullptr;
    if (::getaddrinfo(hostName.c_str(), portStr.c_str(), &hints, &res) != 0 || res == nullptr)
        return false;

    bool connected = false;
    for (struct addrinfo* addr = res; addr != nullptr && !connected; addr = addr->ai_next)
    {
        NanoSocketHandle fd = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (fd == invalidSocketHandle)
            continue;

        // Enable TCP_NODELAY to reduce latency for small OCP.1 frames.
        int one = 1;
        ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                     reinterpret_cast<const char*>(&one), sizeof(one));

        // Non-blocking connect for timeout support.
        m_fd = fd;
        setNonBlocking(true);

        int ret = ::connect(fd, addr->ai_addr, static_cast<int>(addr->ai_addrlen));

#if defined(_WIN32) || defined(_WIN64)
        bool inProgress = (ret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK);
#else
        bool inProgress = (ret < 0 && errno == EINPROGRESS);
#endif

        if (ret == 0 || inProgress)
        {
            // Wait for write-readiness to confirm the connection attempt finished.
            fd_set writeSet, exceptSet;
            FD_ZERO(&writeSet);
            FD_ZERO(&exceptSet);
            FD_SET(fd, &writeSet);
            FD_SET(fd, &exceptSet);

            struct timeval tv;
            tv.tv_sec  = timeoutMs / 1000;
            tv.tv_usec = (timeoutMs % 1000) * 1000;

            int sel = ::select(NANOSOCK_NFDS(fd),
                               nullptr, &writeSet, &exceptSet,
                               timeoutMs >= 0 ? &tv : nullptr);

            if (sel > 0 && FD_ISSET(fd, &writeSet))
            {
                // Verify the connection actually succeeded.
                int err = 0;
                socklen_t errLen = sizeof(err);
                ::getsockopt(fd, SOL_SOCKET, SO_ERROR,
                             reinterpret_cast<char*>(&err), &errLen);
                if (err == 0)
                    connected = true;
            }
        }

        if (connected)
        {
            setNonBlocking(false); // Restore blocking mode.
            m_hostName  = hostName;
            m_connected = true;
        }
        else
        {
            NANOSOCK_CLOSE(fd);
            m_fd = invalidSocketHandle;
        }
    }

    ::freeaddrinfo(res);
    return connected;
}

int NanoSocket::read(void* data, int num, bool blockUntilFull)
{
    if (m_fd == invalidSocketHandle) return -1;

    char* buf = static_cast<char*>(data);

    if (!blockUntilFull)
    {
        int n = static_cast<int>(::recv(m_fd, buf, static_cast<size_t>(num), 0));
        if (n <= 0)
            m_connected = false; // 0 = peer closed gracefully, < 0 = socket error
        return n;
    }

    int total = 0;
    while (total < num)
    {
        int n = static_cast<int>(
            ::recv(m_fd, buf + total, static_cast<size_t>(num - total), 0));
        if (n <= 0)
        {
            m_connected = false; // 0 = peer closed gracefully, < 0 = socket error
            if (n == 0) return total; // graceful close
            return -1;
        }
        total += n;
    }
    return total;
}

int NanoSocket::write(const void* data, int dataSize)
{
    if (m_fd == invalidSocketHandle) return -1;
    return static_cast<int>(
        ::send(m_fd, static_cast<const char*>(data),
               static_cast<size_t>(dataSize), 0));
}

// ── Common ────────────────────────────────────────────────────────────────────

void NanoSocket::close()
{
    if (m_fd != invalidSocketHandle)
    {
        NANOSOCK_CLOSE(m_fd);
        m_fd = invalidSocketHandle;
    }
    m_connected = false;
}

bool NanoSocket::isConnected() const
{
    return m_connected && m_fd != invalidSocketHandle;
}

std::string NanoSocket::getHostName() const
{
    return m_hostName;
}

int NanoSocket::waitUntilReady(bool readyForReading, int timeoutMs) const
{
    if (m_fd == invalidSocketHandle) return -1;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(m_fd, &set);

    struct timeval tv;
    tv.tv_sec  = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int ret = readyForReading
        ? ::select(NANOSOCK_NFDS(m_fd), &set, nullptr, nullptr, &tv)
        : ::select(NANOSOCK_NFDS(m_fd), nullptr, &set, nullptr, &tv);

    if (ret < 0) return -1;
    if (ret == 0) return 0;
    return 1;
}

// ── Server ────────────────────────────────────────────────────────────────────

bool NanoSocket::createListener(int portNumber, const std::string& bindAddress)
{
    close();

    m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_fd == invalidSocketHandle) return false;

    int opt = 1;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<const char*>(&opt), sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(static_cast<unsigned short>(portNumber));

    if (bindAddress.empty())
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        if (::inet_pton(AF_INET, bindAddress.c_str(), &addr.sin_addr) != 1)
        {
            close();
            return false;
        }
    }

    if (::bind(m_fd,
               reinterpret_cast<struct sockaddr*>(&addr),
               sizeof(addr)) != 0)
    {
        close();
        return false;
    }

    if (::listen(m_fd, 5) != 0)
    {
        close();
        return false;
    }

    // Retrieve the actual bound port (useful when portNumber == 0).
    struct sockaddr_in bound{};
    socklen_t boundLen = sizeof(bound);
    if (::getsockname(m_fd, reinterpret_cast<struct sockaddr*>(&bound), &boundLen) == 0)
        m_boundPort = ntohs(bound.sin_port);
    else
        m_boundPort = portNumber;

    m_listener = true;
    return true;
}

NanoSocket* NanoSocket::waitForNextConnection() const
{
    if (m_fd == invalidSocketHandle || !m_listener)
        return nullptr;

    // Use a short select timeout so the caller's loop can check its exit flag.
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_fd, &readSet);
    struct timeval tv{0, 100000}; // 100 ms

    int sel = ::select(NANOSOCK_NFDS(m_fd), &readSet, nullptr, nullptr, &tv);
    if (sel <= 0) return nullptr; // timeout or error (e.g., socket was closed)

    struct sockaddr_storage clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    NanoSocketHandle clientFd = ::accept(
        m_fd,
        reinterpret_cast<struct sockaddr*>(&clientAddr),
        &clientLen);

    if (clientFd == invalidSocketHandle)
        return nullptr;

    char addrStr[INET6_ADDRSTRLEN] = {};
    if (clientAddr.ss_family == AF_INET)
    {
        ::inet_ntop(AF_INET,
                    &reinterpret_cast<struct sockaddr_in*>(&clientAddr)->sin_addr,
                    addrStr, sizeof(addrStr));
    }
    else if (clientAddr.ss_family == AF_INET6)
    {
        ::inet_ntop(AF_INET6,
                    &reinterpret_cast<struct sockaddr_in6*>(&clientAddr)->sin6_addr,
                    addrStr, sizeof(addrStr));
    }

    auto* sock       = new NanoSocket();
    sock->m_fd       = clientFd;
    sock->m_hostName = addrStr;
    sock->m_connected = true;
    return sock;
}

int NanoSocket::getBoundPort() const
{
    return m_boundPort;
}

} // namespace NanoOcp1
