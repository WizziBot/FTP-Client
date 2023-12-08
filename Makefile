CC = g++
CFLAGS = -Wall -Werror

FILES = client.cpp ControlConnection.cpp DataConnection.cpp Commands.cpp
INCLUDE = src/
OBJ = $(FILES:.cpp=.o)

all: client
	
client: client.o ControlConnection.o DataConnection.o Commands.o
	$(CC) $(CFLAGS) ./obj/client.o ./obj/ControlConnection.o ./obj/DataConnection.o ./obj/Commands.o -ggdb -o ./bin/$@ -lstdc++fs

%.o: ./src/%.cpp
	$(CC) $(CFLAGS) $< -c -I$(INCLUDE) -ggdb -o ./obj/$@

clean:
	rm -f $(OBJ)