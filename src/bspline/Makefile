.PHONY: build test

geometry.o : geometry.cpp geometry.h Makefile
	@g++ -g -c geometry.cpp

tests.o : tests.cpp geometry.h Makefile
	@g++ -g -c tests.cpp

build : geometry.o

tests : tests.o build Makefile
	@g++ -g tests.o geometry.o -o tests

test : build tests
	@./tests
