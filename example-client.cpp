/* -----------------------------------------------------
 * Easy WebSocket Client by: D.H. Baird  &  P.B. Plugge.
 * -----------------------------------------------------
 */

#include <iostream>
#include "easywsclient.hpp"


using namespace std;


int main(){
	easywsclient client;
	string msg;

	cout << "connecting...\n";
	client.Connect("ws://chartingexpert.com:8999");

	cout << "sending message..\n";
	client.Send("hi there!");

	sleep(1);

	cout << "receiving message\n";
	if (client.Receive(msg)){
		cout << "received: " << msg << "\n";
	}

	if (!client.Receive(msg)){
		cout << "nothing more to receive\n";
	}

	cout << "Disconnecting\n";
	client.Disconnect();

	cout << "sending message..\n";
	client.Send("This cannot be send!");

	cout << "Done\n";

	return 0;
}
