/* $Id$ */
static char const _copyright[] =
"Copyright © 2012-2020 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Terminal */
static char const _license[] =
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions are\n"
"met:\n"
"\n"
"1. Redistributions of source code must retain the above copyright notice,\n"
"   this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright notice,\n"
"   this list of conditions and the following disclaimer in the documentation\n"
"   and/or other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS \"AS IS\" AND ANY\n"
"EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n"
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
"DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY\n"
"DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n"
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n"
"ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF\n"
"THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";
/* TODO:
 * - figure how to handle X resources */



#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
# include <gtk/gtkx.h>
#endif
#include <System.h>
#include <Desktop.h>
#include "terminal.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

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
typedef struct _TerminalTab TerminalTab;

struct _Terminal
{
	char * shell;
	char * directory;
	unsigned int login;

	/* internal */
	TerminalTab ** tabs;
	size_t tabs_cnt;

	/* widgets */
	GtkWidget * window;
	gboolean fullscreen;
#ifndef EMBEDDED
	GtkWidget * menubar;
#endif
	GtkToolItem * tb_fullscreen;
	GtkWidget * notebook;
};

struct _TerminalTab
{
	Terminal * terminal;
	GtkWidget * widget;
	GtkWidget * label;
	GtkWidget * socket;
	GPid pid;
	guint source;
};


/* constants */
#ifndef EMBEDDED
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};
#endif


/* prototypes */
/* useful */
static int _terminal_open_tab(Terminal * terminal);
static int _terminal_open_window(Terminal * terminal);
static void _terminal_close_tab(Terminal * terminal, unsigned int i);
static void _terminal_close_all(Terminal * terminal);

/* callbacks */
static void _terminal_on_child_watch(GPid pid, gint status, gpointer data);
static void _terminal_on_close(gpointer data);
static gboolean _terminal_on_closex(gpointer data);
static void _terminal_on_fullscreen(gpointer data);
static void _terminal_on_new_tab(gpointer data);
static void _terminal_on_new_window(gpointer data);
static void _terminal_on_tab_close(gpointer data);
static void _terminal_on_tab_rename(gpointer data);

#ifndef EMBEDDED
static void _terminal_on_file_close(gpointer data);
static void _terminal_on_file_close_all(gpointer data);
static void _terminal_on_file_new_tab(gpointer data);
static void _terminal_on_file_new_window(gpointer data);
static void _terminal_on_view_fullscreen(gpointer data);
static void _terminal_on_help_about(gpointer data);
static void _terminal_on_help_contents(gpointer data);
#endif


/* constants */
#ifndef EMBEDDED
/* menubar */
static const DesktopMenu _terminal_file_menu[] =
{
	{ N_("New _tab"), G_CALLBACK(_terminal_on_file_new_tab), "tab-new",
		GDK_CONTROL_MASK, GDK_KEY_T },
	{ N_("_New window"), G_CALLBACK(_terminal_on_file_new_window),
		"window-new", GDK_CONTROL_MASK, GDK_KEY_N },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_terminal_on_file_close), GTK_STOCK_CLOSE,
		GDK_CONTROL_MASK, GDK_KEY_W },
	{ N_("Close all tabs"), G_CALLBACK(_terminal_on_file_close_all), NULL,
		GDK_SHIFT_MASK | GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _terminal_view_menu[] =
{
	{ N_("_Fullscreen"), G_CALLBACK(_terminal_on_view_fullscreen),
# if GTK_CHECK_VERSION(2, 8, 0)
		GTK_STOCK_FULLSCREEN,
# else
		NULL,
# endif
		0, GDK_KEY_F11 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _terminal_help_menu[] =
{
	{ N_("_Contents"), G_CALLBACK(_terminal_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("_About"), G_CALLBACK(_terminal_on_help_about), GTK_STOCK_ABOUT, 0,
		0 },
#else
	{ N_("_About"), G_CALLBACK(_terminal_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _terminal_menubar[] =
{
	{ N_("_File"), _terminal_file_menu },
	{ N_("_View"), _terminal_view_menu },
	{ N_("_Help"), _terminal_help_menu },
	{ NULL, NULL }
};
#endif

static DesktopToolbar _terminal_toolbar[] =
{
	{ N_("New tab"), G_CALLBACK(_terminal_on_new_tab), "tab-new", 0, 0,
		NULL },
	{ N_("New window"), G_CALLBACK(_terminal_on_new_window), "window-new",
		0, 0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* terminal_new */
Terminal * terminal_new(TerminalPrefs * prefs)
{
	Terminal * terminal;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkToolItem * toolitem;

	if((terminal = object_new(sizeof(*terminal))) == NULL)
		return NULL;
	terminal->shell = (prefs != NULL && prefs->shell != NULL)
		? string_new(prefs->shell) : NULL;
	terminal->directory = (prefs != NULL && prefs->directory != NULL)
		? string_new(prefs->directory) : NULL;
	terminal->login = (prefs != NULL) ? prefs->login : 0;
	terminal->tabs = NULL;
	terminal->tabs_cnt = 0;
	terminal->window = NULL;
	terminal->fullscreen = FALSE;
	/* check for errors */
	if((prefs != NULL && prefs->shell != NULL && terminal->shell == NULL)
			|| (prefs != NULL && prefs->directory != NULL
				&& terminal->directory == NULL))
	{
		terminal_delete(terminal);
		return NULL;
	}
	/* widgets */
	group = gtk_accel_group_new();
	terminal->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(terminal->window), group);
	g_object_unref(group);
	gtk_window_set_default_size(GTK_WINDOW(terminal->window), 600, 400);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(terminal->window), "terminal");
#endif
	gtk_window_set_title(GTK_WINDOW(terminal->window), _("Terminal"));
	g_signal_connect_swapped(terminal->window, "delete-event", G_CALLBACK(
				_terminal_on_closex), terminal);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#ifndef EMBEDDED
	/* menubar */
	terminal->menubar = desktop_menubar_create(_terminal_menubar, terminal,
			group);
	gtk_box_pack_start(GTK_BOX(vbox), terminal->menubar, FALSE, TRUE, 0);
#endif
	/* toolbar */
	widget = desktop_toolbar_create(_terminal_toolbar, terminal, group);
#if GTK_CHECK_VERSION(2, 8, 0)
	toolitem = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_FULLSCREEN);
#else
	toolitem = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT);
#endif
	terminal->tb_fullscreen = toolitem;
	g_signal_connect_swapped(G_OBJECT(toolitem), "toggled", G_CALLBACK(
				_terminal_on_fullscreen), terminal);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
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
	{
		if(terminal->tabs[i]->source > 0)
			g_source_remove(terminal->tabs[i]->source);
		if(terminal->tabs[i]->pid > 0)
			g_spawn_close_pid(terminal->tabs[i]->pid);
		free(terminal->tabs[i]);
	}
	/* FIXME also take care of the sub-processes */
	if(terminal->window != NULL)
		gtk_widget_destroy(terminal->window);
	free(terminal->tabs);
	string_delete(terminal->directory);
	string_delete(terminal->shell);
	object_delete(terminal);
}


/* accessors */
/* terminal_set_fullscreen */
void terminal_set_fullscreen(Terminal * terminal, gboolean fullscreen)
{
	if(fullscreen)
	{
#ifndef EMBEDDED
		gtk_widget_hide(terminal->menubar);
#endif
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(
					terminal->tb_fullscreen), TRUE);
		gtk_window_fullscreen(GTK_WINDOW(terminal->window));
	}
	else
	{
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(
					terminal->tb_fullscreen), FALSE);
		gtk_window_unfullscreen(GTK_WINDOW(terminal->window));
#ifndef EMBEDDED
		gtk_widget_show(terminal->menubar);
#endif
	}
	terminal->fullscreen = fullscreen;
}


/* private */
/* functions */
/* useful */
/* terminal_open_tab */
static int _terminal_open_tab(Terminal * terminal)
{
	TerminalTab ** p;
	TerminalTab * tab;
	GtkWidget * widget;
	char * argv[] = { BINDIR "/xterm", "xterm", "-into", NULL,
		"-class", "Terminal", NULL, NULL, NULL };
	char buf[32];
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO
		| G_SPAWN_DO_NOT_REAP_CHILD;
	GError * error = NULL;

	if((p = realloc(terminal->tabs, sizeof(*p) * (terminal->tabs_cnt + 1)))
			== NULL)
		return -1;
	terminal->tabs = p;
	if((tab = malloc(sizeof(*tab))) == NULL)
		return -1;
	terminal->tabs[terminal->tabs_cnt++] = tab;
	/* create the tab */
	tab->terminal = terminal;
	tab->socket = gtk_socket_new();
	tab->widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	tab->label = gtk_label_new(_("xterm"));
	gtk_box_pack_start(GTK_BOX(tab->widget), tab->label, TRUE, TRUE, 0);
	widget = gtk_button_new();
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_terminal_on_tab_rename), tab);
	gtk_container_add(GTK_CONTAINER(widget), gtk_image_new_from_icon_name(
				"gtk-edit", GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(tab->widget), widget, FALSE, TRUE, 0);
	widget = gtk_button_new();
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(
				_terminal_on_tab_close), tab);
	gtk_container_add(GTK_CONTAINER(widget), gtk_image_new_from_icon_name(
				"gtk-close", GTK_ICON_SIZE_MENU));
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(tab->widget), widget, FALSE, TRUE, 0);
	gtk_widget_show_all(tab->widget);
	gtk_notebook_append_page(GTK_NOTEBOOK(terminal->notebook), tab->socket,
			tab->widget);
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(terminal->notebook),
			tab->socket, TRUE);
#endif
	/* launch xterm */
	snprintf(buf, sizeof(buf), "%lu", gtk_socket_get_id(
				GTK_SOCKET(tab->socket)));
	argv[3] = buf;
	if(terminal->login)
	{
		argv[6] = "-ls";
		argv[7] = terminal->shell;
	}
	else
		argv[6] = terminal->shell;
	if(g_spawn_async(terminal->directory, argv, NULL, flags, NULL, NULL,
				&tab->pid, &error) == FALSE)
	{
		fprintf(stderr, "%s: %s: %s\n", PROGNAME_TERMINAL, argv[1],
				error->message);
		g_error_free(error);
		return -1;
	}
	tab->source = g_child_watch_add(tab->pid, _terminal_on_child_watch,
			terminal);
	gtk_widget_show(tab->socket);
	return 0;
}


/* terminal_open_window */
static int _terminal_open_window(Terminal * terminal)
{
	char * argv[] = { BINDIR "/" PROGNAME_TERMINAL, PROGNAME_TERMINAL,
		NULL };
	GSpawnFlags flags = G_SPAWN_FILE_AND_ARGV_ZERO;
	GError * error = NULL;
	(void) terminal;

	if(g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, NULL, &error)
			== FALSE)
	{
		fprintf(stderr, "%s: %s: %s\n", PROGNAME_TERMINAL, argv[0],
				error->message);
		g_error_free(error);
		return -1;
	}
	return 0;
}


/* terminal_close_all */
static void _terminal_close_all(Terminal * terminal)
{
	GPid * pid;
	size_t i;
	size_t cnt;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	gtk_widget_hide(terminal->window);
	/* work on a copy */
	if((pid = malloc(sizeof(*pid) * terminal->tabs_cnt)) == NULL)
	{
		gtk_main_quit();
		return;
	}
	cnt = terminal->tabs_cnt;
	for(i = 0; i < cnt; i++)
	{
		pid[i] = terminal->tabs[i]->pid;
		g_spawn_close_pid(terminal->tabs[i]->pid);
		terminal->tabs[i]->pid = -1;
	}
	/* kill the remaining tabs */
	for(i = 0; i < cnt; i++)
		if(kill(pid[i], SIGTERM) != 0)
			fprintf(stderr, "%s: %s: %s\n", PROGNAME_TERMINAL,
					"kill", strerror(errno));
	free(pid);
	gtk_main_quit();
}


/* terminal_close_tab */
static void _terminal_close_tab(Terminal * terminal, unsigned int i)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, i);
#endif
	if(terminal->tabs[i]->source > 0)
		g_source_remove(terminal->tabs[i]->source);
	if(terminal->tabs[i]->pid >= 0)
	{
		g_spawn_close_pid(terminal->tabs[i]->pid);
		if(kill(terminal->tabs[i]->pid, SIGTERM) != 0)
			fprintf(stderr, "%s: %s: %s\n", PROGNAME_TERMINAL,
					"kill", strerror(errno));
	}
	free(terminal->tabs[i]);
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
		if(terminal->tabs[i]->pid == pid)
			break;
	if(i >= terminal->tabs_cnt)
		return;
	if(WIFEXITED(status))
	{
		if(WEXITSTATUS(status) != 0)
			fprintf(stderr, "%s: %s%u\n", PROGNAME_TERMINAL,
					_("xterm exited with status "),
					WEXITSTATUS(status));
		g_spawn_close_pid(terminal->tabs[i]->pid);
		terminal->tabs[i]->pid = -1;
		_terminal_close_tab(terminal, i);
	}
	else if(WIFSIGNALED(status))
	{
		fprintf(stderr, "%s: %s%u\n", PROGNAME_TERMINAL,
				_("xterm exited with signal "),
				WTERMSIG(status));
		g_spawn_close_pid(terminal->tabs[i]->pid);
		terminal->tabs[i]->pid = -1;
		_terminal_close_tab(terminal, i);
	}
}


/* terminal_on_close */
static void _terminal_on_close(gpointer data)
{
	Terminal * terminal = data;
	int i;

	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
	if(i < 0)
		return;
	_terminal_close_tab(terminal, i);
}


/* terminal_on_closex */
static gboolean _on_closex_confirm(Terminal * terminal);

static gboolean _terminal_on_closex(gpointer data)
{
	Terminal * terminal = data;

	if(terminal->tabs_cnt > 1 && _on_closex_confirm(terminal) != TRUE)
		return TRUE;
	_terminal_close_all(terminal);
	return TRUE;
}

static gboolean _on_closex_confirm(Terminal * terminal)
{
	GtkWidget * dialog;
	gboolean res;

	dialog = gtk_message_dialog_new(GTK_WINDOW(terminal->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Question"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", _("There are multiple tabs opened.\n"
				"Do you really want to close every tab"
				" opened in this window?"));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return (res == GTK_RESPONSE_CLOSE) ? TRUE : FALSE;
}


/* terminal_on_fullscreen */
static void _terminal_on_fullscreen(gpointer data)
{
	Terminal * terminal = data;

	terminal_set_fullscreen(terminal, terminal->fullscreen ? FALSE : TRUE);
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
static void _terminal_on_tab_close(gpointer data)
{
	TerminalTab * tab = data;
	size_t i;

	for(i = 0; i < tab->terminal->tabs_cnt; i++)
		if(tab->terminal->tabs[i]->widget == tab->widget)
			break;
	if(i >= tab->terminal->tabs_cnt)
		/* should not happen */
		return;
	_terminal_close_tab(tab->terminal, i);
}


/* terminal_on_tab_rename */
static void _terminal_on_tab_rename(gpointer data)
{
	TerminalTab * tab = data;
	GtkWidget * dialog;
	GtkWidget * content;
	GtkWidget * entry;
	gchar const * p;

	dialog = gtk_message_dialog_new(GTK_WINDOW(tab->terminal->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Rename tab"));
# if GTK_CHECK_VERSION(2, 10, 0)
	gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog),
			gtk_image_new_from_icon_name("gtk-edit",
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


#ifndef EMBEDDED
/* terminal_on_file_close */
static void _terminal_on_file_close(gpointer data)
{
	Terminal * terminal = data;

	_terminal_on_close(terminal);
}


/* terminal_on_file_close_all */
static void _terminal_on_file_close_all(gpointer data)
{
	Terminal * terminal = data;

	_terminal_close_all(terminal);
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
			_("Terminal for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_logo_icon_name(dialog, "terminal");
	desktop_about_dialog_set_program_name(dialog, PACKAGE);
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_website(dialog, "https://www.defora.org/");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


/* terminal_on_help_contents */
static void _terminal_on_help_contents(gpointer data)
{
	(void) data;

	desktop_help_contents(PACKAGE, PROGNAME_TERMINAL);
}


/* terminal_on_view_fullscreen */
static void _terminal_on_view_fullscreen(gpointer data)
{
	Terminal * terminal = data;

	_terminal_on_fullscreen(terminal);
}
#endif
