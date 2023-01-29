CFLAGS?=-W -Wall -Wextra -O2
CFLAGS+=$(shell gfxprim-config --cflags)
BIN=gpbatinfo
$(BIN): LDLIBS=-lgfxprim $(shell gfxprim-config --libs-widgets)
SOURCES=$(wildcard *.c)
DEP=$(SOURCES:.c=.dep)

all: batinfo $(BIN) $(DEP)

$(BIN): power_supply.o
batinfo: power_supply.o

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

-include: $(DEP)

install:
	install -m 644 -D layout.json $(DESTDIR)/etc/gp_apps/$(BIN)/layout.json
	install -D $(BIN) -t $(DESTDIR)/usr/bin/
	#install -D -m 744 $(BIN).desktop -t $(DESTDIR)/usr/share/applications/
	#install -D -m 644 $(BIN).png -t $(DESTDIR)/usr/share/gpcalc/

clean:
	rm -f $(BIN) batinfo *.dep *.o

