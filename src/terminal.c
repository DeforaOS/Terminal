/* $Id$ */
char const _copyright[] =
"Copyright (c) 2012-2013 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Terminal */
char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";
/* TODO:
 * - figure how to handle X resources */



#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "terminal.h"
#include "../config.h"

/* constants */
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
	guint source;
};


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
/* useful */
static int _terminal_open_tab(Terminal * terminal);
static int _terminal_open_window(Terminal * terminal);
static void _terminal_close_tab(Terminal * terminal, unsigned int i);

/* callbacks */
static void _terminal_on_child_watch(GPid pid, gint status, gpointer data);
static void _terminal_on_close(gpointer data);
static gboolean _terminal_on_closex(gpointer data);
static void _terminal_on_new_tab(gpointer data);
static void _terminal_on_new_window(gpointer data);
static void _terminal_on_tab_close(GtkWidget * widget, gpointer data);

static void _terminal_on_file_close(gpointer data);
static void _terminal_on_file_new_tab(gpointer data);
static void _terminal_on_file_new_window(gpointer data);
static void _terminal_on_help_about(gpointer data);
static void _terminal_on_help_contents(gpointer data);


/* constants */
/* menubar */
static const DesktopMenu _terminal_file_menu[] =
{
	{ "New _tab", G_CALLBACK(_terminal_on_file_new_tab), "tab-new",
		GDK_CONTROL_MASK, GDK_KEY_T },
	{ "_New window", G_CALLBACK(_terminal_on_file_new_window), "window-new",
		GDK_CONTROL_MASK, GDK_KEY_N },
	{ "", NULL, NULL, 0, 0 },
	{ "_Close", G_CALLBACK(_terminal_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _terminal_help_menu[] =
{
	{ "_Contents", G_CALLBACK(_terminal_on_help_contents), "help-contents",
		0, GDK_KEY_F1 },
#if GTK_CHECK_VERSION(2, 6, 0)
	{ "_About", G_CALLBACK(_terminal_on_help_about), GTK_STOCK_ABOUT, 0,
		0 },
#else
	{ "_About", G_CALLBACK(_terminal_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _terminal_menubar[] =
{
	{ "_File", _terminal_file_menu },
	{ "_Help", _terminal_help_menu },
	{ NULL, NULL }
};

static DesktopToolbar _terminal_toolbar[] =
{
	{ "New tab", G_CALLBACK(_terminal_on_new_tab), "tab-new", 0, 0, NULL },
	{ "New window", G_CALLBACK(_terminal_on_new_window), "window-new", 0, 0,
		NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
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

	if((terminal = object_new(sizeof(*terminal))) == NULL)
		return NULL;
	terminal->tabs = NULL;
	terminal->tabs_cnt = 0;
	terminal->window = NULL;
	/* widgets */
	group = gtk_accel_group_new();
	terminal->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(terminal->window), group);
	gtk_window_set_default_size(GTK_WINDOW(terminal->window), 600, 400);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(terminal->window), "terminal");
#endif
	gtk_window_set_title(GTK_WINDOW(terminal->window), "Terminal");
	g_signal_connect_swapped(terminal->window, "delete-event", G_CALLBACK(
				_terminal_on_closex), terminal);
	vbox = gtk_vbox_new(FALSE, 0);
	/* menubar */
	widget = desktop_menubar_create(_terminal_menubar, terminal, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* toolbar */
	widget = desktop_toolbar_create(_terminal_toolbar, terminal, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	terminal->notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(terminal->notebook), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), terminal->notebook, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(terminal->window), vbox);
	gtk_widget_show_all(vbox);
	if(_terminal_open_tab(terminal) != 0)
	{
		terminal_delete(terminal);
		return NULL;
	}
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
/* useful */
/* terminal_open_tab */
static int _terminal_open_tab(Terminal * terminal)
{
	TerminalTab * p;
	GtkWidget * widget;
	char * argv[] = { BINDIR "/xterm", "xterm", "-into", NULL,
		"-class", "Terminal", NULL };
	char buf[16];
	int flags = G_SPAWN_FILE_AND_ARGV_ZERO | G_SPAWN_DO_NOT_REAP_CHILD;
	GError * error = NULL;

	if((p = realloc(terminal->tabs, sizeof(*p) * (terminal->tabs_cnt + 1)))
			== NULL)
		return -1;
	terminal->tabs = p;
	p = &terminal->tabs[terminal->tabs_cnt++];
	/* create the tab */
	p->socket = gtk_socket_new();
	p->label = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(p->label), gtk_label_new("xterm"), TRUE,
			TRUE, 0);
	widget = gtk_button_new();
	g_signal_connect(widget, "clicked", G_CALLBACK(_terminal_on_tab_close),
			terminal);
	gtk_container_add(GTK_CONTAINER(widget), gtk_image_new_from_stock(
				GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(p->label), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(p->label);
	gtk_notebook_append_page(GTK_NOTEBOOK(terminal->notebook), p->socket,
			p->label);
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(terminal->notebook),
			p->socket, TRUE);
#endif
	/* launch xterm */
	snprintf(buf, sizeof(buf), "%u", gtk_socket_get_id(
				GTK_SOCKET(p->socket)));
	argv[3] = buf;
	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &p->pid, &error)
			== FALSE)
	{
		fprintf(stderr, "%s: %s: %s\n", "Terminal", argv[1],
				error->message);
		g_error_free(error);
		return -1;
	}
	p->source = g_child_watch_add(p->pid, _terminal_on_child_watch,
			terminal);
	gtk_widget_show(p->socket);
	return 0;
}


/* terminal_open_window */
static int _terminal_open_window(Terminal * terminal)
{
	char * argv[] = { BINDIR "/terminal", "terminal", NULL };
	int flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			== FALSE)
	{
		fprintf(stderr, "%s: %s: %s\n", "Terminal", argv[0],
				error->message);
		g_error_free(error);
		return -1;
	}
	return 0;
}


/* terminal_close_tab */
static void _terminal_close_tab(Terminal * terminal, unsigned int i)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, i);
#endif
	if(terminal->tabs[i].source > 0)
		g_source_remove(terminal->tabs[i].source);
	if(terminal->tabs[i].pid >= 0)
		g_spawn_close_pid(terminal->tabs[i].pid);
	gtk_notebook_remove_page(GTK_NOTEBOOK(terminal->notebook), i);
	memmove(&terminal->tabs[i], &terminal->tabs[i + 1],
			(terminal->tabs_cnt - (i + 1))
			* sizeof(*terminal->tabs));
	if(--terminal->tabs_cnt == 0)
		gtk_main_quit();
}


/* callbacks */
/* terminal_on_child_watch */
static void _terminal_on_child_watch(GPid pid, gint status, gpointer data)
{
	Terminal * terminal = data;
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %d)\n", __func__, pid, status);
#endif
	for(i = 0; i < terminal->tabs_cnt; i++)
		if(terminal->tabs[i].pid == pid)
			break;
	if(i == terminal->tabs_cnt)
		return;
	if(WIFEXITED(status))
	{
		if(WEXITSTATUS(status) != 0)
			fprintf(stderr, "%s: %s%u\n", "Terminal",
					"xterm exited with status ",
					WEXITSTATUS(status));
		_terminal_close_tab(terminal, i);
	}
	else if(WIFSIGNALED(status))
	{
		fprintf(stderr, "%s: %s%u\n", "Terminal",
				"xterm exited with signal ", WTERMSIG(status));
		_terminal_close_tab(terminal, i);
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


/* terminal_on_new_tab */
static void _terminal_on_new_tab(gpointer data)
{
	Terminal * terminal = data;

	_terminal_open_tab(terminal);
}


/* terminal_on_new_window */
static void _terminal_on_new_window(gpointer data)
{
	Terminal * terminal = data;

	_terminal_open_window(terminal);
}


/* terminal_on_tab_close */
static void _terminal_on_tab_close(GtkWidget * widget, gpointer data)
{
	Terminal * terminal = data;
	size_t i;

	widget = gtk_widget_get_parent(widget);
	for(i = 0; i < terminal->tabs_cnt; i++)
		if(terminal->tabs[i].label == widget)
			break;
	if(i == terminal->tabs_cnt)
		/* should not happen */
		return;
	_terminal_close_tab(terminal, i);
}


/* terminal_on_file_close */
static void _terminal_on_file_close(gpointer data)
{
	Terminal * terminal = data;

	_terminal_on_close(terminal);
}


/* terminal_on_file_new_tab */
static void _terminal_on_file_new_tab(gpointer data)
{
	Terminal * terminal = data;

	_terminal_open_tab(terminal);
}


/* terminal_on_file_new_window */
static void _terminal_on_file_new_window(gpointer data)
{
	Terminal * terminal = data;

	_terminal_open_window(terminal);
}


/* terminal_on_help_about */
static void _terminal_on_help_about(gpointer data)
{
	Terminal * terminal = data;
	GtkWidget * dialog;

	dialog = desktop_about_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(dialog),
			GTK_WINDOW(terminal->window));
	desktop_about_dialog_set_authors(dialog, _authors);
	desktop_about_dialog_set_comments(dialog,
			"Terminal for the DeforaOS desktop");
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_logo_icon_name(dialog, "terminal");
	desktop_about_dialog_set_name(dialog, PACKAGE);
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog, "http://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* terminal_on_help_contents */
static void _terminal_on_help_contents(gpointer data)
{
	desktop_help_contents(PACKAGE, "terminal");
}
