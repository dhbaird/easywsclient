CXXFLAGS = -std=gnu++0x -Wall
LDLIBS = -lstdc++
all: example-client-cpp11 example-client
example-client-cpp11.o: example-client-cpp11.cpp easywsclient.cpp easywsclient.hpp
example-client.o: example-client.cpp easywsclient.cpp easywsclient.hpp
