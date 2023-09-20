CC = g++
CFLAGS = -Wall -Werror

FILES = client.cpp
OBJ = $(FILES:.cpp=)

all: $(OBJ)
	
%: %.cpp
	$(CC) $(CFLAGS) $< -o ./bin/$@

clean:
	rm -f $(OBJ)