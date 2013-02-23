TARGETS	= terminal.1 terminal.html
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

terminal.1: terminal.xml
	./docbook.sh -P "$(PREFIX)" -- "terminal.1"

terminal.html: terminal.xml
	./docbook.sh -P "$(PREFIX)" -- "terminal.html"

clean:
	$(RM) -- $(terminal.1_OBJS) $(terminal.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "terminal.html"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "terminal.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "terminal.html"

uninstall:
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "terminal.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "terminal.html"

.PHONY: all clean distclean install uninstall
