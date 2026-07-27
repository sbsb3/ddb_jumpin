CC ?= gcc
CFLAGS += -std=c11 -fPIC -Wall -O2 -I../deadbeef/include
LDFLAGS += -shared

OUT = ddb_jumpin.so

all: $(OUT)

$(OUT): ddb_jumpin.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

install: $(OUT)
	mkdir -p $(HOME)/.local/lib/deadbeef
	cp $(OUT) $(HOME)/.local/lib/deadbeef/

clean:
	rm -f $(OUT)

.PHONY: all install clean
