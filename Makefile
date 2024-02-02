CC = g++
CFLAGS = -Wall -Werror

FILES = ftp.cpp ControlConnection.cpp DataConnection.cpp Commands.cpp
INCLUDE = src/
OBJ = $(FILES:.cpp=.o)

all: ftp
	
ftp: ftp.o ControlConnection.o DataConnection.o Commands.o
	$(CC) $(CFLAGS) ./obj/ftp.o ./obj/ControlConnection.o ./obj/DataConnection.o ./obj/Commands.o -ggdb -o ./bin/$@ -lstdc++fs

%.o: ./src/%.cpp
	$(CC) $(CFLAGS) $< -c -I$(INCLUDE) -ggdb -o ./obj/$@

clean:
	rm -f $(OBJ)