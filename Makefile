# Build an executable named mysh from mysh.cpp
all: mysh.cpp
	g++ -g -Wall -o mysh mysh.cpp

clean:
	$(RM) mysh
