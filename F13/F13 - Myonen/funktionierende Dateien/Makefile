# specify compiler and flags
CXX = c++
CXXFLAGS += -Wall -O2 -g
# Solaris make uses CC to link C++ files, so we set CC to c++ as well
# to make sure the right libraries are linked
CC = $(CXX)

# build with all root libs
ROOTCFLAGS	 = $(shell root-config --cflags)
ROOTLIBS	 = $(shell root-config --libs)
CXXFLAGS	+= $(ROOTCFLAGS)
LDFLAGS		+= $(ROOTLIBS)

# just calling make will build fp13
all: fp13

# clean up: remove old object files and the like
clean:
	rm -f *.o fp13

# specify the dependencies of the files - make will figure out the rest
fp13: fp13.o fp13Analysis.o logstream.o
fp13.o: fp13.cc fp13Analysis.h logstream.h
fp13Analysis.o: fp13Analysis.cc fp13Analysis.h logstream.h
logstream.o: logstream.cc logstream.h
