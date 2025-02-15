CC = clang
CFLAGS = -std=c99 -Wall -Wextra
INCLUDE = -I../core/include/

SRC_DIR = src
BIN_DIR = ../bin
SOURCES = $(shell find ${SRC_DIR} -name '*.c')

EXEC_NAME = output

dynamic:
	${CC} ${INCLUDE} ${SOURCES} -o ${EXEC_NAME} ${BIN_DIR}/lib_sound_connect.so

static:
	${CC} ${INCLUDE} ${SOURCES} -o ${EXEC_NAME} ${BIN_DIR}/lib_sound_connect.a

clean:
	rm -f ${EXEC_NAME}