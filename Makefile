CC = g++
CFLAGS = -Wall -Werror

FILES = client.cpp ControlConnection.cpp DataConnection.cpp
INCLUDE = src/
OBJ = $(FILES:.cpp=.o)

all: client
	
client: client.o ControlConnection.o DataConnection.o
	$(CC) $(CFLAGS) ./obj/client.o ./obj/ControlConnection.o ./obj/DataConnection.o -ggdb -o ./bin/$@

%.o: ./src/%.cpp
	$(CC) $(CFLAGS) $< -c -I$(INCLUDE) -ggdb -o ./obj/$@

clean:
	rm -f $(OBJ)