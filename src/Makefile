TARGETS	= $(OBJDIR)terminal
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libDesktop` -lintl
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

terminal_OBJS = $(OBJDIR)terminal.o $(OBJDIR)main.o
terminal_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
terminal_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)terminal: $(terminal_OBJS)
	$(CC) -o $(OBJDIR)terminal $(terminal_OBJS) $(terminal_LDFLAGS)

$(OBJDIR)terminal.o: terminal.c terminal.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(terminal_CFLAGS) -o $(OBJDIR)terminal.o -c terminal.c

$(OBJDIR)main.o: main.c terminal.h ../config.h
	$(CC) $(terminal_CFLAGS) -o $(OBJDIR)main.o -c main.c

clean:
	$(RM) -- $(terminal_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)terminal $(DESTDIR)$(BINDIR)/terminal

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/terminal

.PHONY: all clean distclean install uninstall
