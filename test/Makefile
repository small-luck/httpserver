CXX = g++
CXXFLAGS = -g -Wall -W -std=c++11 -lpthread -I../inc/

ifeq ($(debug), yes)
	CXXFLAGS += -DDEBUG
endif

EXEC = test_log
SRCDIR = ../src/

test_log: test_log.cpp ${SRCDIR}log.cpp ${SRCDIR}utils.cpp
	${CXX} ${CXXFLAGS} $^ -o $@

clean:
	rm -rf *.o  ${SRCDIR}*.o ${EXEC}


