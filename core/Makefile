CC = clang
CFLAGS = -std=c99 -Wall -Wextra -pedantic -fPIC
INCLUDE = -Iinclude

SRC_DIR = src
BIN_DIR = ../bin
SOURCES = $(shell find $(SRC_DIR) -name '*.c')

dynamic: lib_sound_connect.so

static: lib_sound_connect.o
	ar r ${BIN_DIR}/lib_sound_connect.a ${BIN_DIR}/lib_sound_connect.o

lib_sound_connect.so: ${SOURCES}
	${CC} ${CFLAGS} ${INCLUDE} -shared -D API_EXPORT -o ${BIN_DIR}/$@ $^

lib_sound_connect.o: ${SOURCES}
	${CC} ${CFLAGS} ${INCLUDE} -c -o ${BIN_DIR}/$@ $^

clean:
	rm -f ${BIN_DIR}/*.so ${BIN_DIR}/*.o ${BIN_DIR}/*.a