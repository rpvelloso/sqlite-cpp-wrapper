CC = gcc
CXXFLAGS = -Wall -O2 -ggdb -std=c++1y
CFLAGS = -Wall -O2 -ggdb
objects = main.o sqlite3.o

all		:	test
test	:	$(objects)
	g++ -std=c++1y -ggdb -Wall $(objects) -o test

main.o		:	sqlite.c++.h sqlite3.h
sqlite3.o	:	sqlite3.h

.PHONY		:	clean
clean		:
	rm -rf $(objects) test.exe
