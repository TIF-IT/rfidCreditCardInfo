# Linux
PCSC_CFLAGS := $(shell pkg-config --cflags libpcsclite)
LDFLAGS := $(shell pkg-config --libs libpcsclite)

# Mac OS X
#PCSC_CFLAGS := -framework PCSC

CFLAGS += $(PCSC_CFLAGS)

ccinfo:	ccinfo.c

clean:
	rm -f ccinfo
