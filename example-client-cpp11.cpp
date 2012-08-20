// Compile with:
// g++ -std=gnu++0x example-client-cpp11.cpp -o example-client-cpp11
#define EASYWSCLIENT_COMPILATION_UNIT
#include "easywsclient.hpp"
#include <assert.h>
#include <stdio.h>
#include <string>

int main()
{
    using easywsclient::WebSocket;
    WebSocket::pointer ws = WebSocket::from_url("ws://localhost:8126/foo");
    assert(ws);
    ws->send("goodbye");
    ws->send("hello");
    while (true) {
        std::string message;
        ws->poll();
        ws->dispatch([&message](const std::string & message_) {
            printf(">>> %s\n", message_.c_str());
            message = message_;
        });
        if (message == "world") { break; }
    }
    return 0;
}
