/*
 * -----------------------------------------------------
 * Easy WebSocket Client by: D.H. Baird  &  P.B. Plugge.
 * -----------------------------------------------------
 */
#include <fcntl.h>			// set socket options with fcntl.
#include <netdb.h>			// addrinfo, getaddrinfo
#include <netinet/tcp.h>	// TCP_NODELAY
#include <string.h>			// std::string
#include <sys/socket.h>	// connect
#include <iostream>			// cout
#include <cstdio>			// sscanf
#include <errno.h>			// errno

#include "easywsclient.hpp"

#define socketerrno errno
#define SOCKET_EAGAIN_EINPROGRESS EAGAIN
#define SOCKET_EWOULDBLOCK EWOULDBLOCK


typedef enum readyStateValues {
	CLOSING,
	CLOSED,
	CONNECTING,
	OPEN
} readyStateValues;


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


/*
 * Constructor.
 */
easywsclient::easywsclient(){
	readyState = CLOSED;
}


/*
 * Destructor.
 */
easywsclient::~easywsclient(){
	Disconnect();
}


/*
 * Connect to host on given port.
 *
 * returns < 0 on failure otherwise the socket fd which you should not have to need.
 */
int easywsclient::ConnectSocket(std::string &hostname, int port){
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *p;
	int ret;
	char sport[16];

	sockfd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	snprintf(sport, 16, "%d", port);

	if ((ret = getaddrinfo(hostname.c_str(), sport, &hints, &result)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		return -1;
	}

	for (p = result; p != NULL; p = p->ai_next){
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1){
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1){
			std::cout << "Connect(): Connected\n";
			break;
		}

		Disconnect();
		sockfd = -1;
	}

	freeaddrinfo(result);

	return sockfd;
}


/*
 * Connect to the url in any way you want..
 * This should all work:
 *
 * Connect("http://hostname.com:8888");
 * Connect("http://hostname.com",8888);
 * Connect(std::string("http://hostname.com:8888"));
 * Connect(std::string("http://hostname.com"),8888);
 */
int easywsclient::Connect(const std::string &url, int prt){
	char host[128];
	int port = prt;
	char path[128];

	if (url.size() >= 128){
		fprintf(stderr, "ERROR: url size limit exceeded: %s\n", url.c_str());
		return -1;
	}

	if (sscanf(url.c_str(), "ws://%[^:/]:%d/%s", host, &port, path) == 3){
	} else if (sscanf(url.c_str(), "ws://%[^:/]/%s", host, path) == 2){
		port = 80;
	} else if (sscanf(url.c_str(), "ws://%[^:/]:%d", host, &port) == 2){
		path[0] = '\0';
	} else if (sscanf(url.c_str(), "ws://%[^:/]", host) == 1){
		port = 80;
		path[0] = '\0';
	} else {
		fprintf(stderr, "ERROR: Could not parse WebSocket url: %s\n",	url.c_str());
		return -1;
	}

	/////////////////////
	// Connect to host.//
	/////////////////////
	fprintf(stderr, "easywsclient: connecting: host=%s port=%d path=/%s\n", host, 	port, path);
	std::string hosts(host);
	if (ConnectSocket(hosts, port) < 0){
		fprintf(stderr, "Unable to connect to %s:%d\n", host, port);
		return -1;
	}
	{
		// XXX: this should be done non-blocking,
		char line[256];
		int status;
		int i;
		snprintf(line, 256, "GET /%s HTTP/1.1\r\n", path);
		::send(sockfd, line, strlen(line), 0);
		if (port == 80) {
			snprintf(line, 256, "Host: %s\r\n", host);
			::send(sockfd, line, strlen(line), 0);
		} else {
			snprintf(line, 256, "Host: %s:%d\r\n", host, port);
			::send(sockfd, line, strlen(line), 0);
		}
		snprintf(line, 256, "Upgrade: websocket\r\n");
		::send(sockfd, line, strlen(line), 0);
		snprintf(line, 256, "Connection: Upgrade\r\n");
		::send(sockfd, line, strlen(line), 0);
		/*if (!origin.empty()) {
			snprintf(line, 256, "Origin: %s\r\n", origin.c_str());
			::send(sockfd, line, strlen(line), 0);
		}*/
		snprintf(line, 256, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n");
		::send(sockfd, line, strlen(line), 0);
		snprintf(line, 256, "Sec-WebSocket-Version: 13\r\n");
		::send(sockfd, line, strlen(line), 0);
		snprintf(line, 256, "\r\n");
		::send(sockfd, line, strlen(line), 0);
		for (i = 0; 	i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n');	++i) {
			if (recv(sockfd, line + i, 1, 0) == 0) {
				return NULL;
			}
		}
		line[i] = 0;
		if (i == 255){
			fprintf(stderr, "ERROR: Got invalid status line connecting to: %s\n", url.c_str());
			return NULL;
		}
		if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101){
			fprintf(stderr, "ERROR: Got bad status connecting to %s: %s",	url.c_str(), line);
			return NULL;
		}
		// TODO: verify response headers,
		while (true){
			for (i = 0; 	i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n');	++i) {
				if (recv(sockfd, line + i, 1, 0) == 0) {
					return NULL;
				}
			}
			if (line[0] == '\r' && line[1] == '\n') {
				break;
			}
		}
	}

	int flag = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(flag)); // Disable Nagle's algorithm

	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	fprintf(stderr, "* Connected to: %s\n", url.c_str());

	readyState = OPEN;

	return sockfd;
}


/*
 * Disconnect the websocket.
 */
void easywsclient::Disconnect(void){
	if (readyState == CLOSED)
		return;

	readyState = CLOSED;

	timeval tv = { 1 / 1000, (1 % 1000) * 1000 };
	select(0, NULL, NULL, NULL, &tv);
}


/*
 * Returns true if connected.
 */
bool easywsclient::Connected(void){
	if (sockfd > 0){
		readyState = OPEN;
		return true;
	}
	return false;
}


/*
 * Returns received message in message.
 *
 * On success returns true.
 */
bool easywsclient::Receive(std::string &message){
	if (readyState == CLOSED){
		std::cout << "easywsclient::Receive(): Not connected.\n";
		return false;
	}

	// receive.
	while (true){
		// FD_ISSET(0, &rfds) will be true.
		int N = rxbuf.size();
		ssize_t ret;
		rxbuf.resize(N + 1500);
		ret = recv(sockfd, (char*) &rxbuf[0] + N, 1500, 0);

		if (ret < 0	&& (socketerrno == SOCKET_EWOULDBLOCK || socketerrno == SOCKET_EAGAIN_EINPROGRESS)){
			rxbuf.resize(N);
			break;
		} else if (ret <= 0){
			rxbuf.resize(N);
			Disconnect();
			readyState = CLOSED;
			fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stderr);
			return false;
		} else {
			rxbuf.resize(N + ret);
		}
	}

	// Make sense of the received message(s).
	// TODO: consider acquiring a lock on rxbuf...
	while (true){
		wsheader_type ws;
		if (rxbuf.size() < 2){
			return false; /* Need at least 2 */
		}
		const uint8_t * data = (uint8_t *) &rxbuf[0]; // peek, but don't consume
		ws.fin = (data[0] & 0x80) == 0x80;
		ws.opcode = (wsheader_type::opcode_type) (data[0] & 0x0f);
		ws.mask = (data[1] & 0x80) == 0x80;
		ws.N0 = (data[1] & 0x7f);
		ws.header_size = 2 + (ws.N0 == 126 ? 2 : 0) + (ws.N0 == 127 ? 6 : 0)
				+ (ws.mask ? 4 : 0);
		if (rxbuf.size() < ws.header_size) {
			return false; /* Need: ws.header_size - rxbuf.size() */
		}
		int i;
		if (ws.N0 < 126) {
			ws.N = ws.N0;
			i = 2;
		} else if (ws.N0 == 126) {
			ws.N = 0;
			ws.N |= ((uint64_t) data[2]) << 8;
			ws.N |= ((uint64_t) data[3]) << 0;
			i = 4;
		} else if (ws.N0 == 127) {
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
			ws.masking_key[0] = ((uint8_t) data[i + 0]) << 0;
			ws.masking_key[1] = ((uint8_t) data[i + 1]) << 0;
			ws.masking_key[2] = ((uint8_t) data[i + 2]) << 0;
			ws.masking_key[3] = ((uint8_t) data[i + 3]) << 0;
		} else {
			ws.masking_key[0] = 0;
			ws.masking_key[1] = 0;
			ws.masking_key[2] = 0;
			ws.masking_key[3] = 0;
		}
		if (rxbuf.size() < ws.header_size + ws.N) {
			return false; /* Need: ws.header_size+ws.N - rxbuf.size() */
		}

		// We got a whole message, now do something with it:
		if (ws.opcode == wsheader_type::TEXT_FRAME && ws.fin){
			if (ws.mask){
				for (size_t i = 0; i != ws.N; ++i){
					rxbuf[i + ws.header_size] ^= ws.masking_key[i & 0x3];
				}
			}
			std::string data(rxbuf.begin() + ws.header_size, rxbuf.begin() + ws.header_size + (size_t) ws.N);
			//callback((const std::string) data);
			message = data;

			rxbuf.erase(rxbuf.begin(), rxbuf.begin() + ws.header_size + (size_t) ws.N);
			return true;
		} else if (ws.opcode == wsheader_type::PING) {
			if (ws.mask) {
				for (size_t i = 0; i != ws.N; ++i) {
					rxbuf[i + ws.header_size] ^= ws.masking_key[i & 0x3];
				}
			}
			std::string data(rxbuf.begin() + ws.header_size, rxbuf.begin() + ws.header_size + (size_t) ws.N);
			SendData(wsheader_type::PONG, data);
		} else if (ws.opcode == wsheader_type::PONG){
		} else if (ws.opcode == wsheader_type::CLOSE){
			Disconnect();
		} else {
			fprintf(stderr, "ERROR: Got unexpected WebSocket message.\n");
			Disconnect();
		}

		rxbuf.erase(rxbuf.begin(), rxbuf.begin() + ws.header_size + (size_t) ws.N);
	}

	return false;
}


/*
 * Sending ping.
 */
void easywsclient::SendPing(){
	SendData(wsheader_type::PING, std::string());
}


/*
 * Sending message.
 */
void easywsclient::Send(const std::string& message){
	SendData(wsheader_type::TEXT_FRAME, message);
}


/*
 * Main send function.
 */
void easywsclient::SendData(int type, const std::string& message) {
	// TODO:
	// Masking key should (must) be derived from a high quality random
	// number generator, to mitigate attacks on non-WebSocket friendly
	// middleware:
	const uint8_t masking_key[4] = { 0x12, 0x34, 0x56, 0x78 };
	// TODO: consider acquiring a lock on txbuf...
	if (readyState == CLOSING || readyState == CLOSED){
		fprintf(stderr, "ERROR: Trying to send data when not connected.\n");
		return;
	}
	std::vector<uint8_t> header;
	uint64_t message_size = message.size();
	header.assign(2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + (useMask ? 4 : 0), 0);
	header[0] = 0x80 | type;
	if (false) {
	} else if (message_size < 126) {
		header[1] = (message_size & 0xff) | (useMask ? 0x80 : 0);
		if (useMask) {
			header[2] = masking_key[0];
			header[3] = masking_key[1];
			header[4] = masking_key[2];
			header[5] = masking_key[3];
		}
	} else if (message_size < 65536){
		header[1] = 126 | (useMask ? 0x80 : 0);
		header[2] = (message_size >> 8) & 0xff;
		header[3] = (message_size >> 0) & 0xff;
		if (useMask) {
			header[4] = masking_key[0];
			header[5] = masking_key[1];
			header[6] = masking_key[2];
			header[7] = masking_key[3];
		}
	} else { // TODO: run coverage testing here
		header[1] = 127 | (useMask ? 0x80 : 0);
		header[2] = (message_size >> 56) & 0xff;
		header[3] = (message_size >> 48) & 0xff;
		header[4] = (message_size >> 40) & 0xff;
		header[5] = (message_size >> 32) & 0xff;
		header[6] = (message_size >> 24) & 0xff;
		header[7] = (message_size >> 16) & 0xff;
		header[8] = (message_size >> 8) & 0xff;
		header[9] = (message_size >> 0) & 0xff;
		if (useMask) {
			header[10] = masking_key[0];
			header[11] = masking_key[1];
			header[12] = masking_key[2];
			header[13] = masking_key[3];
		}
	}
	// N.B. - txbuf will keep growing until it can be transmitted over the socket:
	txbuf.insert(txbuf.end(), header.begin(), header.end());
	txbuf.insert(txbuf.end(), message.begin(), message.end());
	if (useMask){
		for (size_t i = 0; i != message.size(); ++i){
			*(txbuf.end() - message.size() + i) ^= masking_key[i & 0x3];
		}
	}

	// send what is in the buffer.
	while (txbuf.size()){
		std::cout << "Readystate: Sending\n";

		int ret = ::send(sockfd, (char*) &txbuf[0], txbuf.size(), 0);

		if (ret < 0	&& (socketerrno == SOCKET_EWOULDBLOCK || socketerrno == SOCKET_EAGAIN_EINPROGRESS)){
			break;
		} else if (ret <= 0) {
			Disconnect();
			readyState = CLOSED;
			fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stderr);
			break;
		} else {
			txbuf.erase(txbuf.begin(), txbuf.begin() + ret);
		}
	}
}

