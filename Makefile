CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wformat -g -Wshadow
TARGET = tinyc

SRCS = src/main.c src/lexer.c src/parser.c src/codegen.c
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c 
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean test

test: $(TARGET)
	./test.sh

clean:
	rm -rf build $(TARGET)
