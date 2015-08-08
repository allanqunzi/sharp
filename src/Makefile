include Makefile.thrift.inc

CXX=g++
CXXFLAGS=-DIB_USE_STD_STRING -Wall -Wno-switch -g -std=c++0x

BASE_SRC_DIR=../PosixClient/src
INCLUDES=-I../PosixClient/Shared/ -I${BASE_SRC_DIR} -Ithrift
LDLIBS += $(PROTOCOL_LIBS) -lboost_thread -lboost_system -lpthread

TARGET=server



$(TARGET): server.cpp PosixTestClient.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o EClientSocketBase.o -c $(BASE_SRC_DIR)/EClientSocketBase.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o EPosixClientSocket.o -c $(BASE_SRC_DIR)/EPosixClientSocket.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o PosixTestClient.o -c PosixTestClient.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o algotrade-thrift.o -c algotrade-thrift.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o server.o -c server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o client.o -c client.cpp
	$(CXX) $(LDFLAGS) server.o PosixTestClient.o EClientSocketBase.o EPosixClientSocket.o $(PROTOCOL_OBJS) -o $@ $(LDLIBS)
	$(CXX) $(LDFLAGS) client.o PosixTestClient.o EClientSocketBase.o EPosixClientSocket.o $(PROTOCOL_OBJS) -o client $(LDLIBS)

clean:
	rm -f $(TARGET) *.o

