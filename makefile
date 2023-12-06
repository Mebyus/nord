SHELL = bash

-include .build.env

BIN_NAME = nord

RELEASE_DIR = release
DEBUG_DIR = debug

# Compiler executable
CC = g++

CSTANDARD = 17
OPTIMIZATION = 2

# Preprocessor flags
CPPFLAGS = -MMD -MP -MF

# Compiler warnings
WARNINGS = -Wall -Wextra -Wconversion -Wunreachable-code -Wshadow -Wundef -Wfloat-equal -Wformat=2 \
-Wpointer-arith -Winit-self -Wduplicated-branches -Wduplicated-cond -Wnull-dereference -Wswitch-enum -Wvla \
-Wnoexcept -Wswitch-default

# Compiler code generation conventions flags
GENFLAGS = -fwrapv -fno-exceptions

ifeq (${BUILD_KIND}, debug)
	BIN_DIR = bin/debug
	CACHE_DIR = cache/debug
	CFLAGS = ${GENFLAGS} ${WARNINGS} -Werror -pipe -std=c++${CSTANDARD} -g
else
	BIN_DIR = bin/release
	CACHE_DIR = cache/release
	CFLAGS = ${GENFLAGS} ${WARNINGS} -Werror -pipe -std=c++${CSTANDARD} -O${OPTIMIZATION}
endif

BIN_PATH = ${BIN_DIR}/${BIN_NAME}

.PHONY: default
default: build

.PHONY: build
build: dirs ${BIN_PATH}

.PHONY: dirs
dirs: ${BIN_DIR} ${CACHE_DIR}

${BIN_DIR}:
	mkdir -p ${BIN_DIR}

${CACHE_DIR}:
	mkdir -p ${CACHE_DIR}

${BIN_PATH}: ${CACHE_DIR}/main.o
	${CC} -o $@ $^

${CACHE_DIR}/main.o: src/main.cpp makefile
	${CC} ${CPPFLAGS} ${CACHE_DIR}/main.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/main.d

.PHONY: clean
clean:
	rm -rf bin cache
