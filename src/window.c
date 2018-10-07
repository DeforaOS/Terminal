/* $Id$ */
/* Copyright (c) 2018 Pierre Pronchery <khorben@defora.org> */
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



#include <libintl.h>
#include <System.h>
#include "widget.h"
#include "window.h"
#define _(string) gettext(string)


/* TerminalWindow */
/* private */
/* types */
struct _TerminalWindow
{
	Terminal * terminal;
	TerminalWidget * widget;
	gboolean fullscreen;

	/* widgets */
	GtkWidget * window;
};


/* prototypes */
/* callbacks */
static gboolean _terminalwindow_on_closex(gpointer data);


/* public */
/* functions */
/* terminalwindow_new */
TerminalWindow * terminalwindow_new(Terminal * terminal)
{
	TerminalWindow * window;
	GtkWidget * widget;

	if((window = object_new(sizeof(*window))) == NULL)
		return NULL;
	window->terminal = terminal;
	window->fullscreen = FALSE;
	window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if((window->widget = terminalwidget_new(terminal, window->window))
			== NULL)
	{
		terminalwindow_delete(window);
		return NULL;
	}
	/* XXX really default to 80x24 (or a terminal size) */
	gtk_window_set_default_size(GTK_WINDOW(window->window), 600, 400);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(window->window), "terminal");
#endif
	gtk_window_set_title(GTK_WINDOW(window->window), _("Terminal"));
	g_signal_connect_swapped(window->window, "delete-event", G_CALLBACK(
				_terminalwindow_on_closex), window);
	widget = terminalwidget_get_widget(window->widget);
	gtk_container_add(GTK_CONTAINER(window->window), widget);
	gtk_widget_show(window->window);
	return window;
}


/* terminalwindow_delete */
void terminalwindow_delete(TerminalWindow * window)
{
	if(window->widget != NULL)
		terminalwidget_delete(window->widget);
	gtk_widget_destroy(window->window);
	object_delete(window);
}


/* accessors */
/* terminalwindow_get_fullscreen */
gboolean terminalwindow_get_fullscreen(TerminalWindow * window)
{
	return window->fullscreen;
}


/* terminalwindow_get_widget */
TerminalWidget * terminalwindow_get_widget(TerminalWindow * window)
{
	return window->widget;
}


/* terminalwindow_get_widget */
GtkWidget * terminalwindow_get_window(TerminalWindow * window)
{
	return window->window;
}


/* terminalwindow_set_fullscreen */
void terminalwindow_set_fullscreen(TerminalWindow * window, gboolean fullscreen)
{
	if(fullscreen == window->fullscreen)
		return;
	window->fullscreen = fullscreen;
	terminalwidget_set_fullscreen(window->widget, fullscreen);
	if(fullscreen)
		gtk_window_fullscreen(GTK_WINDOW(window->window));
	else
		gtk_window_unfullscreen(GTK_WINDOW(window->window));
}


/* useful */
/* terminalwindow_show_about */
void terminalwindow_show_about(TerminalWindow * terminal, gboolean show)
{
	terminalwidget_show_about(terminal->widget, show);
}


/* terminalwindow_show_preferences */
void terminalwindow_show_preferences(TerminalWindow * terminal, gboolean show)
{
	terminalwidget_show_preferences(terminal->widget, show);
}


/* private */
/* callbacks */
/* terminalwindow_on_closex */
static gboolean _terminalwindow_on_closex(gpointer data)
{
	TerminalWindow * window = data;

	terminalwidget_tab_close_all(window->widget);
	return TRUE;
}
