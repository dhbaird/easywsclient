easywsclient
============

A header-only WebSocket client for C++. Depends only on the standard libraries.
Can make optional use of C++11 features (i.e. std::function and lambda).

Usage
=====

The interface looks somewhat like this:

    // Factory method to create a WebSocket:
    static pointer from_url(std::string url);

    // Function to perform actual network send()/recv() I/O:
    void poll();

    // Receive a message, and pass it to callable(). Really, this just looks at
    // a buffer (filled up by poll()) and decodes any messages in the buffer.
    // Callable must have signature: void(const std::string & message).
    // Should work with C functions, C++ functors, and C++11 std::function and
    // lambda:
    template<class Callable>
    void dispatch(Callable & callable);

    // Sends a TEXT type message (gets put into a buffer for poll() to send
    // later):
    void send(std::string message);

Put altogether, this will look something like this:

    using easywsclient::WebSocket;
    WebSocket::pointer ws = WebSocket::from_url("ws://localhost:8126/foo");
    assert(ws);
    while (true) {
        ws->poll();
        ws->send("hello");
        ws->dispatch(handle_message);
        // ...do more stuff...
    }

Example
=======

    # Launch the server
    node example-server.js

    # Build and launch the client
    g++ example-client.cpp -o example-client
    ./example-client

    # Optional: build and launch a C++11 client
    g++ -std=gnu++0x example-client-cpp11.cpp -o example-client-cpp11
    ./example-client-cpp11

    # Expect the output from example-client:
    Connected to: ws://localhost:8126/foo
    >>> galaxy
    >>> world

Threading
=========

This library is not thread safe. The user must take care to use locks if
accessing an instance of `WebSocket` from multiple threads.

Future Work
===========

(contributions appreciated!)

* Parameterize the `pointer` type (especially for `shared_ptr`).
* Support optional integration on top of an async (event-driven) library,
  especially Asio.
