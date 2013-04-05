easywsclient
============

Easywsclient is an easy and powerful WebSocket client to C++ get
your code connected to a web stack right away. It depends only on
the standard libraries.  It makes use of C++11 std::function and
[lambda](http://en.wikipedia.org/wiki/Anonymous_function#C.2B.2B),
if they're available (it's not required though).  [RFC
6455](http://tools.ietf.org/html/rfc6455) Version 13 WebSocket is
supported this version is compatible with all major, modern WebSocket
implementations, including Node.js.

Rationale: This library is intended to help a C++ project start using
WebSocket rapidly. This small library can easily be thrown into an
existing project. For complicated builds that you can't figure out right
away, you can even cheat by piggy-backing the .cpp file into one of the
project's existing files. Yes, WebSocket is awesome enough to warrant
getting it integrated into your project!  This project imposes no special
interface requirements, and can work happily with new C++11 features or
with older C++ projects.

As an additional benefit, easywsclient is very simple, with just a single
implementation file and can serve as a cruft-free concise reference. You
are most welcome to use this code as a reference for creating alternative
implementations that may better suit your needs.

Usage
=====

The WebSocket class interface looks like this:

```c++
// Factory method to create a WebSocket:
static pointer from_url(std::string url);
// Factory method to create a dummy WebSocket (all operations are noop):
static pointer create_dummy();

// Function to perform actual network send()/recv() I/O:
void poll();

// Receive a message, and pass it to callable(). Really, this just looks at
// a buffer (filled up by poll()) and decodes any messages in the buffer.
// Callable must have signature: void(const std::string & message).
// Should work with C functions, C++ functors, and C++11 std::function and
// lambda:
template<class Callable>
void dispatch(Callable callable);

// Sends a TEXT type message (gets put into a buffer for poll() to send
// later):
void send(std::string message);
```

Put together, the usage looks like this:

```c++
#include "easywsclient.hpp"
#include "easywsclient.cpp" // <-- include only if you don't want compile separately

int
main()
{
    ...
    using easywsclient::WebSocket;
    WebSocket::pointer ws = WebSocket::from_url("ws://localhost:8126/foo");
    assert(ws);
    while (true) {
        ws->poll();
        ws->send("hello");
        ws->dispatch(handle_message);
        // ...do more stuff...
    }
    ...
}
```

Example
=======

    # Launch a test server:
    node example-server.js

    # Build and launch the client:
    g++ example-client.cpp -o example-client
    ./example-client

    # ...or build and launch a C++11 client:
    g++ -std=gnu++0x example-client-cpp11.cpp -o example-client-cpp11
    ./example-client-cpp11

    # Expect the output from example-client:
    Connected to: ws://localhost:8126/foo
    >>> galaxy
    >>> world

Threading
=========

This library is not thread safe. The user must take care to use locks if
accessing an instance of `WebSocket` from multiple threads. If you need
a quick threading library and don't have Boost or something else already,
I recommend [TinyThread++](http://tinythreadpp.bitsnbites.eu/).

Future Work
===========

(contributions appreciated!)

* Parameterize the `pointer` type (especially for `shared_ptr`).
* Support optional integration on top of an async (event-driven) library,
  especially Asio.
