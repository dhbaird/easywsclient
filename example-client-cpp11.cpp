// Compile with:
// g++ -std=gnu++0x example-client-cpp11.cpp -o example-client-cpp11
#include "easywsclient.hpp"
#include "easywsclient.cpp" // <-- include only if you don't want compile separately
#include <assert.h>
#include <stdio.h>
#include <string>

int main()
{
    using easywsclient::WebSocket;
    WebSocket::pointer ws = WebSocket::from_url("ws://localhost:8126/foo");
    assert(ws);
    ws->send("goodbye");
    while (true) {
        ws->poll();
        ws->dispatch([](const std::string & message) {
            printf(">>> %s\n", message.c_str());
            ws->close();
        });
    }
    return 0;
}
