/*
 * -----------------------------------------------------
 * Easy WebSocket Client by: D.H. Baird  &  P.B. Plugge.
 * -----------------------------------------------------
 *
 * Examples below use a client that is created like:
 * easywsclient client;
 *
 *
 * Examples to connect to a websocket server:
 *
 * client.Connect("http://hostname.com:8888");
 * client.Connect("http://hostname.com",8888);
 * client.Connect(std::string("http://hostname.com:8888"));
 * client.Connect(std::string("http://hostname.com"),8888);
 * client.Connect("192.168.1.1",282);
 *
 * Any form should work, this is called easy websockets man!
 *
 * Same with sending:
 *
 * client.Send("hey man!");
 * client.Send(std::string("yes i am here!"));
 *
 * When connected we need to check periodicly for messages.
 *
 * This function returns true if the message was filled with the received message.
 * It returns false if there was no new message.
 *
 *	bool client.Receive(message);
 *
 */

#ifndef EASYWSCLIENT_HPP
#define EASYWSCLIENT_HPP

#include <string>			// string
#include <vector>			// vector
#include <stdint.h>			// uint64_t uint8_t


class easywsclient {
private:
	// State of the socket.
	int readyState;

	// socket descriptor.
	int sockfd;

	bool useMask;

	// the receive buffer.
	std::vector<uint8_t> rxbuf;

	// the send buffer.
	std::vector<uint8_t> txbuf;

	void SendData(int type, const std::string& message);
	int ConnectSocket(std::string &url, int port = -1);
public:
	easywsclient();
	~easywsclient();

	// Connect in any way you want.
	int Connect(const std::string &url, int port = -1);

	// Disconnect.
	void Disconnect(void);

	// returns true if we are connected.
	bool Connected(void);

	// Various functions to send stuff.
	void SendPing(void);
	void Send(const std::string& message);

	// check if we received a message.
	bool Receive(std::string &message);
};


#endif

