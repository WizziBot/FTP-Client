CC = g++
CFLAGS = -Wall -Werror

FILES = client.cpp server.cpp
OBJ = $(FILES:.cpp=)

all: $(OBJ)
	
%: %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ)