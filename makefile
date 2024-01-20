SHELL = bash

-include .build.env

BIN_NAME = nord

RELEASE_DIR = release
DEBUG_DIR = debug

# Compiler executable
CC = g++

CSTANDARD = 20
OPTIMIZATION = fast

# Preprocessor flags
CPPFLAGS = -MMD -MP -MF

# Compiler warnings
WARNINGS = -Wall -Wextra -Wconversion -Wunreachable-code -Wshadow -Wundef -Wfloat-equal -Wformat=2 \
-Wpointer-arith -Winit-self -Wduplicated-branches -Wduplicated-cond -Wnull-dereference -Wswitch-enum -Wvla \
-Wnoexcept -Wswitch-default -Wno-main -Wno-shadow -Wshadow=local

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
build: dirs info ${BIN_DIR}/nord ins flatfit mimic ${BIN_DIR}/game

.PHONY: dirs
dirs: ${BIN_DIR} ${CACHE_DIR}

.PHONY: info
info:
	@echo "${BUILD_KIND} build"

.PHONY: mimic
mimic: ${BIN_DIR}/mimic

.PHONY: math
math: ${CACHE_DIR}/math.o

.PHONY: ins
ins: ${BIN_DIR}/ins

.PHONY: flatfit
flatfit: ${BIN_DIR}/flatfit

${BIN_DIR}:
	@mkdir -p ${BIN_DIR}

${CACHE_DIR}:
	@mkdir -p ${CACHE_DIR}

${BIN_DIR}/nord: ${CACHE_DIR}/main.o
	@${CC} -o $@ $^

${BIN_DIR}/mimic: ${CACHE_DIR}/mimic.o
	@${CC} -o $@ $^

${BIN_DIR}/ins: ${CACHE_DIR}/input_sequence_inspect.o
	@${CC} -o $@ $^

${BIN_DIR}/flatfit: ${CACHE_DIR}/flat_fit.o ${CACHE_DIR}/syscall_linux_amd64.o
	@${CC} -o $@ $^

${BIN_DIR}/game: ${CACHE_DIR}/game.o
	@${CC} -o $@ $^ -lSDL2

${CACHE_DIR}/main.o: src/main.cpp makefile
	@${CC} ${CPPFLAGS} ${CACHE_DIR}/main.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/main.d

${CACHE_DIR}/mimic.o: src/mimic.cpp makefile
	@${CC} ${CPPFLAGS} ${CACHE_DIR}/mimic.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/mimic.d

${CACHE_DIR}/input_sequence_inspect.o: src/input_sequence_inspect.cpp makefile
	@${CC} ${CPPFLAGS} ${CACHE_DIR}/input_sequence_inspect.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/input_sequence_inspect.d

${CACHE_DIR}/flat_fit.o: src/flat_fit.cpp makefile
	@${CC} ${CPPFLAGS} ${CACHE_DIR}/flat_fit.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/flat_fit.d

${CACHE_DIR}/game.o: src/game.cpp makefile
	@${CC} ${CPPFLAGS} ${CACHE_DIR}/game.d ${CFLAGS} -I./vendor/include -o $@ -c $<
-include ${CACHE_DIR}/game.d

${CACHE_DIR}/math.o: src/math.cpp makefile
	@${CC} ${CPPFLAGS} ${CACHE_DIR}/math.d ${CFLAGS} -o $@ -c $<
-include ${CACHE_DIR}/math.d

${CACHE_DIR}/syscall_linux_amd64.o: src/syscall_linux_amd64.s
	@as -o $@ $<

.PHONY: clean
clean:
	@rm -rf bin cache
