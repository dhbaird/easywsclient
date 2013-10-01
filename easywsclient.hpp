#ifndef EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD
#define EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD

// This code comes from:
// https://github.com/dhbaird/easywsclient
//
// To get the latest version:
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.hpp
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.cpp

#include <string>

namespace easywsclient {

class WebSocket {
  public:
    typedef WebSocket * pointer;
    typedef enum readyStateValues { CLOSING, CLOSED, CONNECTING, OPEN } readyStateValues;

    // Factories:
    static pointer create_dummy();
    static pointer from_url(std::string url);

    // Interfaces:
    virtual ~WebSocket() { }
    virtual void poll() = 0;
    virtual void send(std::string message) = 0;
    virtual void close() = 0;
    virtual readyStateValues getReadyState() = 0;
    template<class Callable>
    void dispatch(Callable callable) { // N.B. this is compatible with both C++11 lambdas, functors and C function pointers
        struct _Callback : public Callback {
            Callable & callable;
            _Callback(Callable & callable) : callable(callable) { }
            void operator()(const std::string & message) { callable(message); }
        };
        _Callback callback(callable);
        _dispatch(callback);
    }

  protected:
    struct Callback { virtual void operator()(const std::string & message) = 0; };
    virtual void _dispatch(Callback & callable) = 0;
};

} // namespace easywsclient

#endif /* EASYWSCLIENT_HPP_20120819_MIOFVASDTNUASZDQPLFD */
