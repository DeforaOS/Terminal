/* $Id$ */
/* Copyright (c) 2012-2013 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Terminal */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/* TODO:
 * - figure how to handle X resources
 * - determine if XTerm needs the "allowSendEvents" resource */



#include <sys/wait.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "terminal.h"
#include "../config.h"

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif


/* Terminal */
/* private */
/* types */
typedef struct _TerminalTab TerminalTab;

struct _Terminal
{
	/* internal */
	TerminalTab * tabs;
	size_t tabs_cnt;

	/* widgets */
	GtkWidget * window;
	GtkWidget * notebook;
};

struct _TerminalTab
{
	GtkWidget * label;
	GtkWidget * socket;
	GPid pid;
};


/* prototypes */
/* callbacks */
static void _terminal_on_child_watch(GPid pid, gint status, gpointer data);
static gboolean _terminal_on_closex(gpointer data);
static void _terminal_on_close(gpointer data);
static gboolean _terminal_on_closex(gpointer data);

static void _terminal_on_file_close(gpointer data);

/* constants */
/* menubar */
static const DesktopMenu _terminal_file_menu[] =
{
	{ "_Close", G_CALLBACK(_terminal_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _terminal_menubar[] =
{
	{ "_File", _terminal_file_menu },
	{ NULL, NULL }
};


/* public */
/* functions */
/* terminal_new */
Terminal * terminal_new(void)
{
	Terminal * terminal;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;
	char * argv[] = { BINDIR "/xterm", "xterm", "-into", NULL, NULL };
	char buf[16];
	int flags = G_SPAWN_FILE_AND_ARGV_ZERO | G_SPAWN_DO_NOT_REAP_CHILD;
	GError * error = NULL;

	if((terminal = object_new(sizeof(*terminal))) == NULL)
		return NULL;
	/* FIXME really implement */
	terminal->tabs = malloc(sizeof(*terminal->tabs));
	terminal->tabs_cnt = 1;
	terminal->window = NULL;
	/* check for errors */
	if(terminal->tabs == NULL)
	{
		terminal_delete(terminal);
		return NULL;
	}
	/* widgets */
	group = gtk_accel_group_new();
	terminal->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(terminal->window), group);
	gtk_window_set_default_size(GTK_WINDOW(terminal->window), 400, 200);
	gtk_window_set_title(GTK_WINDOW(terminal->window), "Terminal");
	g_signal_connect_swapped(terminal->window, "delete-event", G_CALLBACK(
				_terminal_on_closex), terminal);
	vbox = gtk_vbox_new(FALSE, 0);
	/* menu bar */
	widget = desktop_menubar_create(_terminal_menubar, terminal, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	terminal->notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), terminal->notebook, TRUE, TRUE, 0);
	/* first tab */
	terminal->tabs->socket = gtk_socket_new();
	terminal->tabs->label = gtk_label_new("xterm");
	gtk_notebook_append_page(GTK_NOTEBOOK(terminal->notebook),
			terminal->tabs->socket, terminal->tabs->label);
	gtk_container_add(GTK_CONTAINER(terminal->window), vbox);
	gtk_widget_show_all(vbox);
	/* launch xterm */
	snprintf(buf, sizeof(buf), "%u", gtk_socket_get_id(
				GTK_SOCKET(terminal->tabs->socket)));
	argv[3] = buf;
	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL,
				&terminal->tabs->pid, &error) == FALSE)
	{
		fprintf(stderr, "%s: %s: %s\n", "Terminal", argv[1],
				error->message);
		g_error_free(error);
		terminal_delete(terminal);
		return NULL;
	}
	g_child_watch_add(terminal->tabs->pid, _terminal_on_child_watch,
			terminal);
	gtk_widget_show(terminal->window);
	return terminal;
}


/* terminal_delete */
void terminal_delete(Terminal * terminal)
{
	size_t i;

	for(i = 0; i < terminal->tabs_cnt; i++)
		if(terminal->tabs[i].pid > 0)
			g_spawn_close_pid(terminal->tabs[i].pid);
	/* FIXME also take care of the sub-processes */
	if(terminal->window != NULL)
		gtk_widget_destroy(terminal->window);
	free(terminal->tabs);
	object_delete(terminal);
}


/* private */
/* functions */
/* callbacks */
/* terminal_on_child_watch */
static void _terminal_on_child_watch(GPid pid, gint status, gpointer data)
{
	Terminal * terminal = data;

	if(terminal->tabs->pid != pid)
		return;
	if(WIFEXITED(status))
	{
		if(WEXITSTATUS(status) != 0)
			fprintf(stderr, "%s: %s%u\n", "Terminal",
					"xterm exited with status ",
					WEXITSTATUS(status));
		gtk_main_quit();
	}
	else if(WIFSIGNALED(status))
	{
		fprintf(stderr, "%s: %s%u\n", "Terminal",
				"xterm exited with signal ", WTERMSIG(status));
		gtk_main_quit();
	}
}


/* terminal_on_close */
static void _terminal_on_close(gpointer data)
{
	Terminal * terminal = data;

	gtk_widget_hide(terminal->window);
	gtk_main_quit();
}


/* terminal_on_closex */
static gboolean _terminal_on_closex(gpointer data)
{
	Terminal * terminal = data;

	_terminal_on_close(terminal);
	return TRUE;
}


/* terminal_on_file_close */
static void _terminal_on_file_close(gpointer data)
{
	Terminal * terminal = data;

	_terminal_on_close(terminal);
}
