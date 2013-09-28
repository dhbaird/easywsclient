#include "easywsclient.hpp"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <fcntl.h>
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment( lib, "ws2_32" )
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/types.h>
    #include <io.h>
    #ifndef _SSIZE_T_DEFINED
        typedef int ssize_t;
        #define _SSIZE_T_DEFINED
    #endif
    #ifndef _SOCKET_T_DEFINED
        typedef SOCKET socket_t;
        #define _SOCKET_T_DEFINED
    #endif
    #ifndef snprintf
        #define snprintf _snprintf_s
    #endif
    #if _MSC_VER >=1600
        // vs2010 or later
        #include <stdint.h>
    #else
        typedef __int8 int8_t;
        typedef unsigned __int8 uint8_t;
        typedef __int32 int32_t;
        typedef unsigned __int32 uint32_t;
        typedef __int64 int64_t;
        typedef unsigned __int64 uint64_t;
    #endif
#else
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/tcp.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <stdint.h>
    #ifndef _SOCKET_T_DEFINED
        typedef int socket_t;
        #define _SOCKET_T_DEFINED
    #endif
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET (-1)
    #endif
    #ifndef SOCKET_ERROR
        #define SOCKET_ERROR   (-1)
    #endif
#endif

#include <vector>
#include <string>

namespace { // private module-only namespace

socket_t hostname_connect(std::string hostname, int port) {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *p;
    int ret;
    socket_t sockfd = INVALID_SOCKET;
    char sport[16];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(sport, 16, "%d", port);
    if ((ret = getaddrinfo(hostname.c_str(), sport, &hints, &result)) != 0)
    {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
      return 1;
    }
    for(p = result; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == INVALID_SOCKET) { continue; }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) != SOCKET_ERROR) {
            break;
        }
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        sockfd = INVALID_SOCKET;
    }
    freeaddrinfo(result);
    return sockfd;
}


class _DummyWebSocket : public easywsclient::WebSocket
{
  public:
    void poll() { }
    void send(std::string message) { }
    void close() { } 
    void _dispatch(Callback & callable) { }
    readyStateValues getReadyState() { return CLOSED; }
};



class _RealWebSocket : public easywsclient::WebSocket
{
  public:
    // http://tools.ietf.org/html/rfc6455#section-5.2  Base Framing Protocol
    //
    //  0                   1                   2                   3
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    // +-+-+-+-+-------+-+-------------+-------------------------------+
    // |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    // |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    // |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    // | |1|2|3|       |K|             |                               |
    // +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    // |     Extended payload length continued, if payload len == 127  |
    // + - - - - - - - - - - - - - - - +-------------------------------+
    // |                               |Masking-key, if MASK set to 1  |
    // +-------------------------------+-------------------------------+
    // | Masking-key (continued)       |          Payload Data         |
    // +-------------------------------- - - - - - - - - - - - - - - - +
    // :                     Payload Data continued ...                :
    // + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    // |                     Payload Data continued ...                |
    // +---------------------------------------------------------------+
    struct wsheader_type {
        unsigned header_size;
        bool fin;
        bool mask;
        enum opcode_type {
            CONTINUATION = 0x0,
            TEXT_FRAME = 0x1,
            BINARY_FRAME = 0x2,
            CLOSE = 8,
            PING = 9,
            PONG = 0xa,
        } opcode;
        int N0;
        uint64_t N;
        uint8_t masking_key[4];
    };

    inline int close(socket_t sockfd) {
#ifdef _WIN32
        return ::closesocket(sockfd);
#else
        return ::close(sockfd);
#endif
    }

    std::vector<uint8_t> rxbuf;
    std::vector<uint8_t> txbuf;

    socket_t sockfd;
    readyStateValues readyState;

    _RealWebSocket(socket_t sockfd) : sockfd(sockfd), readyState(OPEN) {
    }

    readyStateValues getReadyState() {
      return readyState;
    }

    void poll() {
        if (readyState == CLOSED) { return; }
        while (true) {
            // FD_ISSET(0, &rfds) will be true
            int N = rxbuf.size();
            ssize_t ret;
            rxbuf.resize(N + 1500);
#ifdef _WIN32
            ret = recv(sockfd, (int8_t*)&rxbuf[0] + N, 1500, 0);
#else
            ret = recv(sockfd, &rxbuf[0] + N, 1500, 0);
#endif
            if (false) { }
            else if (ret < 0) {
                rxbuf.resize(N);
                break;
            }
            else if (ret == 0) {
                rxbuf.resize(N);
                close(sockfd);
                readyState = CLOSED;
                fprintf(stderr, "Connection closed!\n");
                break;
            }
            else {
                rxbuf.resize(N + ret);
            }
        }
        while (txbuf.size()) {
            int ret;
#ifdef _WIN32
			ret = ::send(sockfd, (int8_t*)&txbuf[0], txbuf.size(), 0);
#else
			ret = ::send(sockfd, &txbuf[0], txbuf.size(), 0);
#endif
			if (ret > 0) { txbuf.erase(txbuf.begin(), txbuf.begin() + ret); }
            else { break; }
        }
        if (!txbuf.size() && readyState == CLOSING) {
            close(sockfd);
            readyState = CLOSED;
        }
    }

    // Callable must have signature: void(const std::string & message).
    // Should work with C functions, C++ functors, and C++11 std::function and
    // lambda:
    //template<class Callable>
    //void dispatch(Callable callable)
    virtual void _dispatch(WebSocket::Callback & callable) {
        // TODO: consider acquiring a lock on rxbuf...
        while (true) {
            wsheader_type ws;
            if (rxbuf.size() < 2) { return; /* Need at least 2 */ }
            const uint8_t * data = (uint8_t *) &rxbuf[0]; // peek, but don't consume
            ws.fin = (data[0] & 0x80) == 0x80;
            ws.opcode = (wsheader_type::opcode_type) (data[0] & 0x0f);
            ws.mask = (data[1] & 0x80) == 0x80;
            ws.N0 = (data[1] & 0x7f);
            ws.header_size = 2 + (ws.N0 == 126? 2 : 0) + (ws.N0 == 127? 6 : 0) + (ws.mask? 4 : 0);
            if (rxbuf.size() < ws.header_size) { return; /* Need: ws.header_size - rxbuf.size() */ }
            int i;
            if (ws.N0 < 126) {
                ws.N = ws.N0;
                i = 2;
            }
            else if (ws.N0 == 126) {
                ws.N = 0;
                ws.N |= ((uint64_t) data[2]) << 8;
                ws.N |= ((uint64_t) data[3]) << 0;
                i = 4;
            }
            else if (ws.N0 == 127) {
                ws.N = 0;
                ws.N |= ((uint64_t) data[2]) << 56;
                ws.N |= ((uint64_t) data[3]) << 48;
                ws.N |= ((uint64_t) data[4]) << 40;
                ws.N |= ((uint64_t) data[5]) << 32;
                ws.N |= ((uint64_t) data[6]) << 24;
                ws.N |= ((uint64_t) data[7]) << 16;
                ws.N |= ((uint64_t) data[8]) << 8;
                ws.N |= ((uint64_t) data[9]) << 0;
                i = 10;
            }
            if (ws.mask) {
                ws.masking_key[0] = ((uint8_t) data[i+0]) << 0;
                ws.masking_key[1] = ((uint8_t) data[i+1]) << 0;
                ws.masking_key[2] = ((uint8_t) data[i+2]) << 0;
                ws.masking_key[3] = ((uint8_t) data[i+3]) << 0;
            }
            else {
                ws.masking_key[0] = 0;
                ws.masking_key[1] = 0;
                ws.masking_key[2] = 0;
                ws.masking_key[3] = 0;
            }
            if (rxbuf.size() < ws.header_size+ws.N) { return; /* Need: ws.header_size+ws.N - rxbuf.size() */ }

            // We got a whole message, now do something with it:
            if (false) { }
            else if (ws.opcode == wsheader_type::TEXT_FRAME && ws.fin) {
                if (ws.mask) { for (size_t i = 0; i != ws.N; ++i) { rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3]; } }
                std::string data(rxbuf.begin()+ws.header_size, rxbuf.begin()+ws.header_size+ws.N);
                callable((const std::string) data);
            }
            else if (ws.opcode == wsheader_type::PING) { }
            else if (ws.opcode == wsheader_type::PONG) { }
            else if (ws.opcode == wsheader_type::CLOSE) { close(); }
            else { fprintf(stderr, "ERROR: Got unexpected WebSocket message.\n"); close(); }

            rxbuf.erase(rxbuf.begin(), rxbuf.begin() + ws.header_size+ws.N);
        }
    }

    void send(std::string message) {
        // TODO: consider acquiring a lock on txbuf...
        if (readyState == CLOSING || readyState == CLOSED) { return; }
        std::vector<uint8_t> header;
        uint64_t message_size = message.size();
        header.assign(2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0), 0);
        header[0] = 0x80 | wsheader_type::TEXT_FRAME;
        if (false) { }
        else if (message_size < 126) {
            header[1] = message_size & 0xff;
        }
        else if (message_size < 65536) {
            header[1] = 126;
            header[2] = (message_size >> 8) & 0xff;
            header[3] = (message_size >> 0) & 0xff;
        }
        else { // TODO: run coverage testing here
            header[1] = 127;
            header[2] = (message_size >> 56) & 0xff;
            header[3] = (message_size >> 48) & 0xff;
            header[4] = (message_size >> 40) & 0xff;
            header[5] = (message_size >> 32) & 0xff;
            header[6] = (message_size >> 24) & 0xff;
            header[7] = (message_size >> 16) & 0xff;
            header[8] = (message_size >>  8) & 0xff;
            header[9] = (message_size >>  0) & 0xff;
        }
        txbuf.insert(txbuf.end(), header.begin(), header.end());
        txbuf.insert(txbuf.end(), message.begin(), message.end());
    }

    void close() {
        if(readyState == CLOSING || readyState == CLOSED) { return; }
        readyState = CLOSING;
        uint8_t closeFrame[4] = {0x88, 0x00, 0x00, 0x00};
        std::vector<uint8_t> header(closeFrame, closeFrame+4);
        txbuf.insert(txbuf.end(), header.begin(), header.end());
    }

};



} // end of module-only namespace



namespace easywsclient {

WebSocket::pointer WebSocket::create_dummy() {
    static pointer dummy = pointer(new _DummyWebSocket);
    return dummy;
}

WebSocket::pointer WebSocket::from_url(std::string url) {
    char host[128];
    int port;
    char path[128];
    if (false) { }
    else if (sscanf(url.c_str(), "ws://%[^:/]:%d/%s", host, &port, path) == 3) {
    }
    else if (sscanf(url.c_str(), "ws://%[^:/]/%s", host, path) == 2) {
        port = 80;
    }
    else if (sscanf(url.c_str(), "ws://%[^:/]:%d", host, &port) == 2) {
        path[0] = '\0';
    }
    else if (sscanf(url.c_str(), "ws://%[^:/]", host) == 1) {
        port = 80;
        path[0] = '\0';
    }
    else {
        fprintf(stderr, "ERROR: Could not parse WebSocket url: %s\n", url.c_str());
        return NULL;
    }
    fprintf(stderr, "easywsclient: connecting: host=%s port=%d path=/%s\n", host, port, path);
    socket_t sockfd = hostname_connect(host, port);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "Unable to connect to %s:%d\n", host, port);
        return NULL;
    }
    {
        // XXX: this should be done non-blocking,
        char line[256];
        int status;
        int i;
        snprintf(line, 256, "GET /%s HTTP/1.1\r\n", path); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "Host: %s:%d\r\n", host, port); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "Upgrade: websocket\r\n"); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "Connection: Upgrade\r\n"); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "Sec-WebSocket-Version: 13\r\n"); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "\r\n"); ::send(sockfd, line, strlen(line), 0);
        for (i = 0; i < 2 || (i < 255 && line[i-2] != '\r' && line[i-1] != '\n'); ++i) { if (recv(sockfd, line+i, 1, 0) == 0) { return NULL; } }
        line[i] = 0;
        if (i == 255) { fprintf(stderr, "ERROR: Got invalid status line connecting to: %s\n", url.c_str()); return NULL; }
        if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101) { fprintf(stderr, "ERROR: Got bad status connecting to %s: %s", url.c_str(), line); return NULL; }
        // TODO: verify response headers,
        while (true) {
            for (i = 0; i < 2 || (i < 255 && line[i-2] != '\r' && line[i-1] != '\n'); ++i) { if (recv(sockfd, line+i, 1, 0) == 0) { return NULL; } }
            if (line[0] == '\r' && line[1] == '\n') { break; }
        }
    }
    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(flag)); // Disable Nagle's algorithm
#ifdef _WIN32
    u_long on = 1;
    ioctlsocket(sockfd, FIONBIO, &on);
#else
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif
    fprintf(stderr, "Connected to: %s\n", url.c_str());
    return pointer(new _RealWebSocket(sockfd));
}


} // namespace easywsclient
