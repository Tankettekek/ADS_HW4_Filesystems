CC = clang 
CFLAGS = -Wall -Wextra -pedantic -ggdb3 -O0

EXECUTABLE = main

# List of source files (adjust as needed)
SOURCES = $(wildcard *.c) 

# Derive object files from source files
OBJECTS = $(SOURCES:.c=.o)

# Default target: build all object files
all: $(EXECUTABLE) 

# Generic rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE)

# Clean target
clean:
	rm -f $(OBJECTS)

