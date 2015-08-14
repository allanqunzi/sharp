
CXX=g++
CXXFLAGS=-DIB_USE_STD_STRING -Wall -Wno-switch -g -std=c++0x

PROTOCOL_OBJS = ib_constants.o ib_types.o Sharp.o

$(PROTOCOL_OBJS): ib_constants.cpp ib_types.cpp Sharp.cpp
	$(CXX) $(CXXFLAGS) -o ib_constants.o -c ib_constants.cpp
	$(CXX) $(CXXFLAGS) -o ib_types.o -c ib_types.cpp
	$(CXX) $(CXXFLAGS) -o Sharp.o -c Sharp.cpp