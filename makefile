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
-Wnoexcept -Wswitch-default -Wno-main

# Compiler code generation conventions flags
GENFLAGS = -fwrapv -fno-exceptions

ifeq (${BUILD_KIND}, debug)
	BIN_DIR = bin/debug
	CACHE_DIR = cache/debug
	CFLAGS = ${GENFLAGS} ${WARNINGS} -Werror -pipe -std=c++${CSTANDARD} -ggdb
else
	BIN_DIR = bin/release
	CACHE_DIR = cache/release
	CFLAGS = ${GENFLAGS} ${WARNINGS} -Werror -pipe -std=c++${CSTANDARD} -O${OPTIMIZATION}
endif

.PHONY: default
default: build

.PHONY: build
build: dirs ${BIN_DIR}/nord ${BIN_DIR}/ins # ${BIN_DIR}/mimic

.PHONY: dirs
dirs: ${BIN_DIR} ${CACHE_DIR}

${BIN_DIR}:
	mkdir -p ${BIN_DIR}

${CACHE_DIR}:
	mkdir -p ${CACHE_DIR}

${BIN_DIR}/nord: ${CACHE_DIR}/main.o
	${CC} -o $@ $^

${BIN_DIR}/mimic: ${CACHE_DIR}/mimic.o
	${CC} -o $@ $^

${BIN_DIR}/ins: ${CACHE_DIR}/input_sequence_inspect.o
	${CC} -o $@ $^

${CACHE_DIR}/main.o: src/main.cpp makefile
	${CC} ${CPPFLAGS} ${CACHE_DIR}/main.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/main.d

${CACHE_DIR}/mimic.o: src/mimic.cpp makefile
	${CC} ${CPPFLAGS} ${CACHE_DIR}/mimic.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/mimic.d

${CACHE_DIR}/input_sequence_inspect.o: src/input_sequence_inspect.cpp makefile
	${CC} ${CPPFLAGS} ${CACHE_DIR}/input_sequence_inspect.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/input_sequence_inspect.d

.PHONY: clean
clean:
	rm -rf bin cache
