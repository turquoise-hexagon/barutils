CC ?= gcc

CFLAGS  += -pedantic -Wall -Wextra
LDFLAGS +=

PREFIX ?= $(DESTDIR)/usr
BINDIR  =  $(PREFIX)/bin

SRC = $(wildcard *.c)
BIN = $(basename $(SRC))

all : CFLAGS += -O3 -march=native
all : $(BIN)

debug : CFLAGS += -O3 -g
debug : $(BIN)

clean :
	rm -f $(BIN)

install : all
	install -Dm755 $(BIN) -t $(BINDIR)

uninstall :
	rm -f $(addprefix $(BINDIR)/,$(BIN))

.PHONY : all debug clean install uninstall
