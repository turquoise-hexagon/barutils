CC     ?= gcc
CFLAGS += -pedantic -Wall -Wextra

BIN = $(shell printf '%s\n' src/*.c | sed 's|^src|bin|g;s|\.c$$||g')

PREFIX ?= $(DESTDIR)/usr
BINDIR  = $(PREFIX)/bin

all : $(BIN) $(MAN)

bin/% : src/%.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $< -o $@

clean :
	rm -rf bin

install : all
	install -Dm755 bin/* -t $(BINDIR)

uninstall :
	@echo "uninstalling binaries..."
	@for src in src/*.c; do \
		src=$${src%.c}; \
		src=$${src##*/}; \
		rm -f $(BINDIR)/"$$src"; \
	done
	

.PHONY: all clean install uninstall
