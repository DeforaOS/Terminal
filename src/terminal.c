/* $Id$ */
/* Copyright (c) 2012-2018 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Terminal */
/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. */
/* TODO:
 * - figure how to handle X resources */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <System.h>
#include <Desktop.h>
#include "tab.h"
#include "widget.h"
#include "window.h"
#include "terminal.h"
#include "../config.h"

/* constants */
#ifndef PROGNAME_TERMINAL
# define PROGNAME_TERMINAL	"terminal"
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR			PREFIX "/bin"
#endif


/* Terminal */
/* private */
/* types */
struct _Terminal
{
	String * shell;
	String * directory;
	unsigned int login;

	TerminalWindow * window;
};


/* public */
/* functions */
/* terminal_new */
Terminal * terminal_new(TerminalPrefs * prefs)
{
	Terminal * terminal;

	if((terminal = object_new(sizeof(*terminal))) == NULL)
		return NULL;
	terminal->shell = (prefs != NULL && prefs->shell != NULL)
		? string_new(prefs->shell) : NULL;
	terminal->directory = (prefs != NULL && prefs->directory != NULL)
		? string_new(prefs->directory) : NULL;
	terminal->login = (prefs != NULL) ? prefs->login : 0;
	terminal->window = NULL;
	/* check for errors */
	if((prefs != NULL && prefs->shell != NULL && terminal->shell == NULL)
			|| (prefs != NULL && prefs->directory != NULL
				&& terminal->directory == NULL)
			|| (terminal->window = terminalwindow_new(terminal))
			== NULL)
	{
		terminal_delete(terminal);
		return NULL;
	}
	terminal_tab_open(terminal);
	return terminal;
}


/* terminal_delete */
void terminal_delete(Terminal * terminal)
{
	if(terminal->window != NULL)
		terminalwindow_delete(terminal->window);
	string_delete(terminal->directory);
	string_delete(terminal->shell);
	object_delete(terminal);
}


/* accessors */
/* terminal_get_directory */
char const * terminal_get_directory(Terminal * terminal)
{
	return terminal->directory;
}


/* terminal_get_shell */
char const * terminal_get_shell(Terminal * terminal)
{
	return terminal->shell;
}


/* terminal_get_window */
TerminalWindow * terminal_get_window(Terminal * terminal)
{
	return terminal->window;
}


/* terminal_is_login */
gboolean terminal_is_login(Terminal * terminal)
{
	return (terminal->login > 0) ? TRUE : FALSE;
}


/* useful */
/* terminal_tab_close */
void terminal_tab_close(Terminal * terminal, TerminalTab * tab)
{
	TerminalWidget * widget;

	widget = terminalwindow_get_widget(terminal->window);
	terminalwidget_tab_close(widget, tab);
}


/* terminal_tab_close_all */
void terminal_tab_close_all(Terminal * terminal)
{
	TerminalWidget * widget;

	widget = terminalwindow_get_widget(terminal->window);
	terminalwidget_tab_close_all(widget);
}


/* terminal_tab_open */
TerminalTab * terminal_tab_open(Terminal * terminal)
{
	TerminalWidget * widget;

	widget = terminalwindow_get_widget(terminal->window);
	return terminalwidget_tab_open(widget);
}


/* terminal_window_open */
int terminal_window_open(Terminal * terminal)
{
	char * argv[] = { BINDIR "/" PROGNAME_TERMINAL, PROGNAME_TERMINAL,
		NULL };
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			== FALSE)
	{
		/* XXX use the current window or a dialog box */
		fprintf(stderr, "%s: %s: %s\n", PROGNAME_TERMINAL, argv[0],
				error->message);
		g_error_free(error);
		return -1;
	}
	return 0;
}
