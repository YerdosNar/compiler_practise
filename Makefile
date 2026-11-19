CC = gcc
TARGET = tinyc
CFLAGS = -std=c11 -Wall -Wextra \
	 -Wpedantic -Wformat -g \
	 -Wshadow -MMD -MP

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

.PHONY: clean test debug help

help:
	@echo "make [options]"
	@echo "Options:"
	@echo "  clean		To clean all build files, and Assembly source codes"
	@echo "  test		To run test cases"
	@echo "  debug  	To compile with DEBUG flag (logs printed to STDERR)"
	@echo "  help		To print this message"

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS+="-DDEBUG -O0"

test: $(TARGET)
	./test.sh

clean:
	rm -rf build $(TARGET) tmp* *.s
