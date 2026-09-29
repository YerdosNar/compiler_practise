CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wformat -g
TARGET = tinyc

SRCS = src/main.c src/lexer.c src/parser.c 
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c 
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf build $(TARGET)
