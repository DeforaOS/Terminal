TARGETS	= terminal
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libDesktop`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

terminal_OBJS = terminal.o main.o
terminal_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
terminal_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

terminal: $(terminal_OBJS)
	$(CC) -o terminal $(terminal_OBJS) $(terminal_LDFLAGS)

terminal.o: terminal.c terminal.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(terminal_CFLAGS) -c terminal.c

main.o: main.c terminal.h ../config.h
	$(CC) $(terminal_CFLAGS) -c main.c

clean:
	$(RM) -- $(terminal_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 -- terminal $(DESTDIR)$(BINDIR)/terminal

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/terminal

.PHONY: all clean distclean install uninstall
