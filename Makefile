CC = g++
CFLAGS = -Wall -Werror

FILES = client.cpp ControlConnection.cpp
INCLUDE = src/
OBJ = $(FILES:.cpp=.o)

all: client
	
client: client.o ControlConnection.o
	$(CC) $(CFLAGS) ./obj/client.o ./obj/ControlConnection.o -ggdb -o ./bin/$@

%.o: ./src/%.cpp
	$(CC) $(CFLAGS) $< -c -I$(INCLUDE) -ggdb -o ./obj/$@

clean:
	rm -f $(OBJ)