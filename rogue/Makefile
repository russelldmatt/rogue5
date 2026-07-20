# Modern POSIX/macOS build for the 4.3BSD Rogue 5 sources.

CC ?= cc
CPPFLAGS += -DUNIX -DUNIX_POSIX
CFLAGS ?= -O2
CFLAGS += -std=gnu89 -fwritable-strings \
	-Wno-implicit-int \
	-Wno-implicit-function-declaration \
	-Wno-deprecated-non-prototype \
	-Wno-int-conversion \
	-Wno-incompatible-library-redeclaration \
	-Wno-incompatible-function-pointer-types \
	-Wno-return-type
LDLIBS += -lncurses

PROG = rogue
SRCS = curses.c hit.c init.c inventory.c level.c machdep.c main.c \
	message.c monster.c move.c object.c pack.c play.c random.c ring.c \
	room.c save.c score.c spec_hit.c throw.c trap.c use.c zap.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c rogue.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(PROG)
	./$(PROG)

clean:
	rm -f $(PROG) $(OBJS) rogue.lock
