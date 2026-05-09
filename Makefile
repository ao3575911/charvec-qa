CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS ?=

BUILD_DIR := build
TARGET := $(BUILD_DIR)/charvec
SOURCES := src/main.c src/charvec.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES) include/charvec.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TARGET)
	sh tests/smoke.sh

clean:
	rm -rf $(BUILD_DIR)

