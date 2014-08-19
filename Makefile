CXXFLAGS = -std=gnu++0x -Wall
LDLIBS = -lstdc++ -lssl -lcrypto
.PHONY: all clean
all: example-client example-client-cpp11
clean:
	-rm  example-client example-secure-client example-client-cpp11 *.o
testserver: node_modules
	node example-server.js
node_modules:
	npm install
example-client-cpp11: example-client-cpp11.o easywsclient.o
example-client-cpp11.o: example-client-cpp11.cpp easywsclient.hpp
example-client: example-client.o easywsclient.o
example-client.o: example-client.cpp easywsclient.hpp
example-secure-client: example-secure-client.o easywsclient.o
example-secure-client.o: example-secure-client.cpp easywsclient.hpp
easywsclient.o: easywsclient.cpp easywsclient.hpp
