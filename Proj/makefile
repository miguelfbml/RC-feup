# Makefile for link_layer and application_layer compilation

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g

# Source files
SRCS = link_layer.c application_layer.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
EXEC = program

# Default target
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) -lm

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
