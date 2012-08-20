#include "easywsclient.hpp"
#include <assert.h>
#include <stdio.h>
#include <string>

// N.B. A real application should abuse a global variable like this...
// (gets messy if threads are involved)
std::string message;

void handle_message(const std::string & message)
{
    printf(">>> %s\n", message.c_str());
    ::message = message;
}

int main()
{
    using easywsclient::WebSocket;
    WebSocket::pointer ws = WebSocket::from_url("ws://localhost:8126/foo");
    assert(ws);
    ws->send("goodbye");
    ws->send("hello");
    while (message != "world") {
        ws->poll();
        ws->dispatch(handle_message);
    }
    return 0;
}
