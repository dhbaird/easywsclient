#include "easywsclient.hpp"
#include "easywsclient.cpp" // <-- include only if you don't want compile separately
#include <assert.h>
#include <stdio.h>
#include <string>

void handle_message(const std::string & message)
{
    printf(">>> %s\n", message.c_str());
}

int main()
{
    using easywsclient::WebSocket;
    WebSocket::pointer ws = WebSocket::from_url("ws://localhost:8126/foo");
    assert(ws);
    ws->send("goodbye");
    ws->send("hello");
    ws->close();
    while(true) {
      ws->poll();
      ws->dispatch(handle_message);
    }
    
    return 0;
}
