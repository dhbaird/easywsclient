easywsclient
============

Easywsclient is a single-file, header-only WebSocket
client for C++. It depends only on the standard libraries.
Can make optional use of C++11 features (i.e. std::function and
[lambda](http://en.wikipedia.org/wiki/Anonymous_function#C.2B.2B)).
Supported is [RFC 6455](http://tools.ietf.org/html/rfc6455) Version
13 WebSocket.

Rationale: This library is intended to help a C++ project start using
WebSocket rapidly.  It does not require your project to derive from any
spooky/mystical interfaces (at most, it may need a functor, but that's
pretty conventional).  It does not require you to be using this-or-that
particular asynchronous library.  It only requires your OS to support
sockets!

However! This is probably not the end-point for your project,
as this library puts a lot of crap into the header file
(easier to use, but eliminates the benefits of [separate
compilation](http://en.wikipedia.org/wiki/Single_Compilation_Unit)).
Also, this library does not work in cooperation with any asynchronous
event processing scheduler. The good news is that the code here is
straightforward and can serve as a reference to build something tailored
for your needs.

Happy hacking! Drop me a line if you do anything cool with this :)
...complaints welcome too.

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
