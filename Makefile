TARGET = mp3meta
SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}
CC = cc
CFLAGS = -Wall
LFLAGS =
DESTDIR = /usr/local
INSTALL = install
INSTALL_ARGS = -o root -g wheel -m 755
INSTALL_DIR = ${DESTDIR}/bin
MANDIR = ${DESTDIR}/man/man1
MAN_INSTALL_ARGS = -o root -g wheel -m 644

VERSION =`awk '{if ($$2 == "VERSION") print $$3}' main.c | awk -F '"' '{print $$2}'`

ifeq (${CC}, $(filter ${CC}, cc clang gcc))
        CFLAGS += -std=c99 -pedantic
endif

all: debug

debug: CFLAGS += -g -DDEBUG
debug: LFLAGS += -g
debug: build

release: CFLAGS += -Os
release: LFLAGS += -s
release: clean build

build: build_host.h ${TARGET}

build_host.h:
	@echo "#define BUILD_HOST \"`hostname`\""      > build_host.h
	@echo "#define BUILD_OS \"`uname`\""          >> build_host.h
	@echo "#define BUILD_PLATFORM \"`uname -m`\"" >> build_host.h
	@echo "#define BUILD_KERNEL \"`uname -r`\""   >> build_host.h

${TARGET}: build_host.h ${OBJ}
	${CC} ${LFLAGS} -o $@ ${OBJ}

%.o : %.c
	${CC} ${CFLAGS} -o $@ -c $?

install: release
	${INSTALL} ${INSTALL_ARGS} ${TARGET} ${INSTALL_DIR}
	mkdir -p ${MANDIR}
	@echo "Installing man page"
	@sed s/VERSION/${VERSION}/ < mp3meta.1 > ${MANDIR}/mp3meta.1
	chmod 644 ${MANDIR}/mp3meta.1
	@echo "DONE"

uninstall:
	-rm -f ${INSTALL_DIR}/${TARGET}
	-rm -f ${MANDIR}/mp3meta.1

clean:
	-rm -f build_host.h
	-rm -f *.o ${TARGET}

.PHONY : all debug release build install clean
