CFLAGS=-W -Wall -Wextra -O2 $(shell gfxprim-config --cflags)
BIN=gpbatinfo batinfo
$(BIN): LDLIBS=-lgfxprim $(shell gfxprim-config --libs-widgets)
SOURCES=$(wildcard *.c)
DEP=$(SOURCES:.c=.dep)

all: $(BIN) $(DEP)

$(BIN): power_supply.o

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

-include: $(DEP)

clean:
	rm -f $(BIN) *.dep *.o

