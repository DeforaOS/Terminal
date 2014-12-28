TARGETS	= $(OBJDIR)Terminal.pot $(OBJDIR)es.mo $(OBJDIR)fr.mo
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)Terminal.pot: POTFILES
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)Terminal.pot"

$(OBJDIR)es.mo: $(OBJDIR)Terminal.pot es.po
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)es.mo"

$(OBJDIR)fr.mo: $(OBJDIR)Terminal.pot fr.po
	./gettext.sh -P "$(PREFIX)" -- "$(OBJDIR)fr.mo"

clean:
	$(RM) -- $(Terminal.pot_OBJS) $(es.mo_OBJS) $(fr.mo_OBJS)
	./gettext.sh -c -P "$(PREFIX)" -- "Terminal.pot"
	./gettext.sh -c -P "$(PREFIX)" -- "es.mo"
	./gettext.sh -c -P "$(PREFIX)" -- "fr.mo"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)es.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)fr.mo"

uninstall:
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -u -- "es.mo"
	./gettext.sh -P "$(DESTDIR)$(PREFIX)" -u -- "fr.mo"

.PHONY: all clean distclean install uninstall
