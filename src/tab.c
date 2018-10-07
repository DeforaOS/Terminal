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
 * 3. Neither the name of the authors nor the names of the contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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



#include <stdio.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#ifdef WITH_XTERM
# if GTK_CHECK_VERSION(3, 0, 0)
#  include <gtk/gtkx.h>
# endif
#else
# include <vte/vte.h>
#endif
#include <System.h>
#include <Desktop.h>
#include "terminal.h"
#include "window.h"
#include "tab.h"
#define _(string) gettext(string)

/* constants */
#ifndef PROGNAME_TERMINAL
# define PROGNAME_TERMINAL	"terminal"
#endif
#ifdef WITH_XTERM
# ifndef PROGNAME_XTERM
#  define PROGNAME_XTERM		"xterm"
# endif
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR			PREFIX "/bin"
#endif


/* TerminalTab */
/* private */
/* types */
struct _TerminalTab
{
	Terminal * terminal;
	GtkWidget * widget;
	GtkWidget * label;
	GtkWidget * socket;
	GPid pid;
	guint source;
};


/* prototypes */
/* callbacks */
static void _terminaltab_on_child_watch(GPid pid, gint status, gpointer data);
static void _terminaltab_on_close(gpointer data);
static void _terminaltab_on_rename(gpointer data);
static void _terminaltab_on_screen_changed(GtkWidget * widget,
		GdkScreen * screen, gpointer data);


/* public */
/* functions */
/* terminaltab_new */
TerminalTab * terminaltab_new(Terminal * terminal)
{
	TerminalTab * tab;
	GtkWidget * widget;

	if((tab = object_new(sizeof(*tab))) == NULL)
		return NULL;
	tab->terminal = terminal;
	tab->pid = -1;
	tab->source = 0;
#ifdef WITH_XTERM
	tab->socket = gtk_socket_new();
#else
	tab->socket = vte_terminal_new();
#endif
	g_signal_connect(tab->socket, "screen-changed", G_CALLBACK(
				_terminaltab_on_screen_changed), tab);
	tab->widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	tab->label = gtk_label_new(_("xterm"));
	gtk_box_pack_start(GTK_BOX(tab->widget), tab->label, TRUE, TRUE, 0);
	widget = gtk_button_new();
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_terminaltab_on_rename), tab);
	gtk_container_add(GTK_CONTAINER(widget), gtk_image_new_from_stock(
				GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(tab->widget), widget, FALSE, TRUE, 0);
	widget = gtk_button_new();
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_terminaltab_on_close), tab);
	gtk_container_add(GTK_CONTAINER(widget), gtk_image_new_from_stock(
				GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(tab->widget), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(tab->widget);
	gtk_widget_show(tab->socket);
	return tab;
}


/* terminaltab_delete */
void terminaltab_delete(TerminalTab * tab)
{
	if(tab->source > 0)
		g_source_remove(tab->source);
	if(tab->pid > 0)
	{
		g_spawn_close_pid(tab->pid);
		if(kill(tab->pid, SIGTERM) != 0)
			fprintf(stderr, "%s: %s: %s\n", PROGNAME_TERMINAL,
					"kill", strerror(errno));
	}
	object_delete(tab);
}


/* accessors */
/* terminaltab_get_label */
GtkWidget * terminaltab_get_label(TerminalTab * tab)
{
	return tab->widget;
}


/* terminaltab_get_widget */
GtkWidget * terminaltab_get_widget(TerminalTab * tab)
{
	return tab->socket;
}


/* useful */
/* terminaltab_clipboard_copy */
void terminaltab_clipboard_copy(TerminalTab * tab)
{
#ifdef WITH_XTERM
#else
	vte_terminal_copy_clipboard(VTE_TERMINAL(tab->socket));
#endif
}


/* terminaltab_clipboard_paste */
void terminaltab_clipboard_paste(TerminalTab * tab)
{
#ifdef WITH_XTERM
#else
	vte_terminal_paste_clipboard(VTE_TERMINAL(tab->socket));
#endif
}


/* private */
/* functions */
/* callbacks */
/* terminaltab_on_child_watch */
static void _terminaltab_on_child_watch(GPid pid, gint status, gpointer data)
{
	TerminalTab * tab = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %d)\n", __func__, pid, status);
#endif
	if(tab->pid != pid)
		return;
	if(WIFEXITED(status))
	{
		if(WEXITSTATUS(status) != 0)
			fprintf(stderr, "%s: %s%u\n", PROGNAME_TERMINAL,
					_("process exited with status "),
					WEXITSTATUS(status));
		g_spawn_close_pid(tab->pid);
		tab->pid = -1;
		terminal_tab_close(tab->terminal, tab);
	}
	else if(WIFSIGNALED(status))
	{
		fprintf(stderr, "%s: %s%u\n", PROGNAME_TERMINAL,
				_("process exited with signal "),
				WTERMSIG(status));
		g_spawn_close_pid(tab->pid);
		tab->pid = -1;
		terminal_tab_close(tab->terminal, tab);
	}
}


/* terminaltab_on_close */
static void _terminaltab_on_close(gpointer data)
{
	TerminalTab * tab = data;

	terminal_tab_close(tab->terminal, tab);
}


/* terminaltab_on_rename */
static void _terminaltab_on_rename(gpointer data)
{
	TerminalTab * tab = data;
	TerminalWindow * twindow;
	GtkWidget * window;
	GtkWidget * dialog;
	GtkWidget * content;
	GtkWidget * entry;
	gchar const * p;

	twindow = terminal_get_window(tab->terminal);
	window = terminalwindow_get_window(twindow);
	dialog = gtk_message_dialog_new(GTK_WINDOW(window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Rename tab"));
# if GTK_CHECK_VERSION(2, 10, 0)
	gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog),
			gtk_image_new_from_stock(GTK_STOCK_EDIT,
				GTK_ICON_SIZE_DIALOG));
# endif
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", _("Rename this tab as:"));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Rename tab"));
#if GTK_CHECK_VERSION(2, 14, 0)
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
#else
	content = GTK_DIALOG(dialog)->vbox;
#endif
	entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
	gtk_entry_set_text(GTK_ENTRY(entry), gtk_label_get_text(
				GTK_LABEL(tab->label)));
	gtk_box_pack_start(GTK_BOX(content), entry, FALSE, TRUE, 0);
	gtk_widget_show_all(content);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		p = gtk_entry_get_text(GTK_ENTRY(entry));
		gtk_label_set_text(GTK_LABEL(tab->label), p);
	}
	gtk_widget_destroy(dialog);
}


/* terminaltab_on_screen_changed */
static void _terminaltab_on_screen_changed(GtkWidget * widget,
		GdkScreen * screen, gpointer data)
{
	TerminalTab * tab = data;
	const GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO
		| G_SPAWN_DO_NOT_REAP_CHILD;
	GError * error = NULL;
	gboolean res;
#ifdef WITH_XTERM
	char * argv[] = { BINDIR "/" PROGNAME_XTERM, PROGNAME_XTERM,
		"-into", NULL, "-class", "Terminal", NULL, NULL, NULL };
	char buf[32];

	if(screen != NULL)
		return;
	/* launch xterm */
	snprintf(buf, sizeof(buf), "%lu", gtk_socket_get_id(
				GTK_SOCKET(widget)));
	argv[3] = buf;
	if(terminal_is_login(tab->terminal))
	{
		argv[6] = "-ls";
		argv[7] = terminal_get_shell(tab->terminal);
	}
	else
		argv[6] = terminal_get_shell(tab->terminal);
	res = g_spawn_async(terminal_get_directory(tab->terminal), argv, NULL,
			flags, NULL, NULL, &tab->pid, &error);
#else
	char * argv[] = { NULL, NULL, NULL };

	if((argv[0] = vte_get_user_shell()) == NULL)
		argv[0] = "/bin/sh";
	argv[1] = g_path_get_basename(argv[0]);
	res = vte_terminal_spawn_sync(VTE_TERMINAL(tab->socket),
			VTE_PTY_DEFAULT, NULL, argv, NULL, flags, NULL, NULL,
			&tab->pid, NULL, &error);
	g_free(argv[1]);
#endif
	if(res == FALSE)
	{
		fprintf(stderr, "%s: %s: %s\n", PROGNAME_TERMINAL, argv[0],
				error->message);
		g_error_free(error);
		return;
	}
	tab->source = g_child_watch_add(tab->pid, _terminaltab_on_child_watch,
			tab);
}
