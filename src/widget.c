/* $Id$ */
static char const _copyright[] =
"Copyright © 2012-2018 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Terminal */
static char const _license[] =
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions are\n"
"met:\n"
"\n"
"1. Redistributions of source code must retain the above copyright\n"
"   notice, this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"   notice this list of conditions and the following disclaimer in the\n"
"   documentation and/or other materials provided with the distribution.\n"
"\n"
"3. Neither the name of the authors nor the names of the contributors may\n"
"   be used to endorse or promote products derived from this software\n"
"   without specific prior written permission.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS \"AS IS\" AND\n"
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR\n"
"PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS\n"
"BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
"CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
"SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
"INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
"CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
"ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF\n"
"THE POSSIBILITY OF SUCH DAMAGE.";



#include <libintl.h>
#include <gdk/gdkkeysyms.h>
#include <System.h>
#include <Desktop.h>
#include "common.h"
#include "tab.h"
#include "terminal.h"
#include "window.h"
#include "widget.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

/* constants */
#ifndef PROGNAME_TERMINAL
# define PROGNAME_TERMINAL	"terminal"
#endif


/* TerminalWidget */
/* private */
/* types */
struct _TerminalWidget
{
	Terminal * terminal;

	TerminalTab ** tabs;
	size_t tabs_cnt;

	/* widgets */
	GtkWidget * widget;
#ifndef EMBEDDED
	GtkWidget * menubar;
#endif
	GtkWidget * about;
	GtkToolItem * fullscreen;
	GtkWidget * notebook;
};


/* prototypes */
static void _terminalwidget_tab_close(TerminalWidget * widget, unsigned int i);

/* callbacks */
static void _terminalwidget_on_close(gpointer data);
static void _terminalwidget_on_fullscreen(gpointer data);
static void _terminalwidget_on_new_tab(gpointer data);
static void _terminalwidget_on_new_window(gpointer data);

#ifndef EMBEDDED
static void _terminalwidget_on_file_close(gpointer data);
static void _terminalwidget_on_file_close_all(gpointer data);
static void _terminalwidget_on_file_new_tab(gpointer data);
static void _terminalwidget_on_file_new_window(gpointer data);
static void _terminalwidget_on_view_fullscreen(gpointer data);
static void _terminalwidget_on_help_about(gpointer data);
static void _terminalwidget_on_help_contents(gpointer data);
#endif


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};

#ifndef EMBEDDED
/* menubar */
static const DesktopMenu _terminalwidget_file_menu[] =
{
	{ N_("New _tab"), G_CALLBACK(_terminalwidget_on_file_new_tab),
		"tab-new", GDK_CONTROL_MASK, GDK_KEY_T },
	{ N_("_New window"), G_CALLBACK(_terminalwidget_on_file_new_window),
		"window-new", GDK_CONTROL_MASK, GDK_KEY_N },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Close"), G_CALLBACK(_terminalwidget_on_file_close),
		GTK_STOCK_CLOSE, GDK_CONTROL_MASK, GDK_KEY_W },
	{ N_("Close all tabs"), G_CALLBACK(_terminalwidget_on_file_close_all),
		NULL, GDK_SHIFT_MASK | GDK_CONTROL_MASK, GDK_KEY_W },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _terminalwidget_view_menu[] =
{
	{ N_("_Fullscreen"), G_CALLBACK(_terminalwidget_on_view_fullscreen),
# if GTK_CHECK_VERSION(2, 8, 0)
		GTK_STOCK_FULLSCREEN,
# else
		NULL,
# endif
		0, GDK_KEY_F11 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _terminalwidget_help_menu[] =
{
	{ N_("_Contents"), G_CALLBACK(_terminalwidget_on_help_contents),
		"help-contents", 0, GDK_KEY_F1 },
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("_About"), G_CALLBACK(_terminalwidget_on_help_about),
		GTK_STOCK_ABOUT, 0, 0 },
#else
	{ N_("_About"), G_CALLBACK(_terminalwidget_on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _terminalwidget_menubar[] =
{
	{ N_("_File"), _terminalwidget_file_menu },
	{ N_("_View"), _terminalwidget_view_menu },
	{ N_("_Help"), _terminalwidget_help_menu },
	{ NULL, NULL }
};
#endif

static DesktopToolbar _terminalwidget_toolbar[] =
{
	{ N_("New tab"), G_CALLBACK(_terminalwidget_on_new_tab), "tab-new", 0,
		0, NULL },
	{ N_("New window"), G_CALLBACK(_terminalwidget_on_new_window),
		"window-new", 0, 0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};


/* public */
/* functions */
/* terminalwidget_new */
TerminalWidget * terminalwidget_new(Terminal * terminal, GtkWidget * window)
{
	TerminalWidget * widget;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * toolbar;
	GtkToolItem * toolitem;

	if((widget = object_new(sizeof(*widget))) == NULL)
		return NULL;
	widget->terminal = terminal;
	widget->tabs = NULL;
	widget->tabs_cnt = 0;
	group = gtk_accel_group_new();
	widget->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	vbox = widget->widget;
#ifndef EMBEDDED
	/* menubar */
	widget->menubar = desktop_menubar_create(_terminalwidget_menubar,
			widget, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget->menubar, FALSE, TRUE, 0);
#endif
	widget->about = NULL;
	/* toolbar */
	toolbar = desktop_toolbar_create(_terminalwidget_toolbar, widget, group);
#if GTK_CHECK_VERSION(2, 8, 0)
	toolitem = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_FULLSCREEN);
#else
	toolitem = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_ZOOM_FIT);
#endif
	widget->fullscreen = toolitem;
	g_signal_connect_swapped(G_OBJECT(toolitem), "toggled", G_CALLBACK(
				_terminalwidget_on_fullscreen), terminal);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	gtk_window_add_accel_group(GTK_WINDOW(window), group);
	g_object_unref(group);
	/* view */
	widget->notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(widget->notebook), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), widget->notebook, TRUE, TRUE, 0);
	gtk_widget_show_all(vbox);
	return widget;
}


/* terminalwidget_delete */
void terminalwidget_delete(TerminalWidget * widget)
{
	size_t i;

	for(i = 0; i < widget->tabs_cnt; i++)
		terminaltab_delete(widget->tabs[i]);
	/* FIXME also take care of the sub-processes */
	free(widget->tabs);
	object_delete(widget);
}


/* accessors */
/* terminalwidget_get_fullscreen */
gboolean terminalwidget_get_fullscreen(TerminalWidget * widget)
{
	TerminalWindow * window;

	window = terminal_get_window(widget->terminal);
	return terminalwindow_get_fullscreen(window);
}


/* terminalwidget_get_widget */
GtkWidget * terminalwidget_get_widget(TerminalWidget * widget)
{
	return widget->widget;
}


/* terminalwidget_set_fullscreen */
void terminalwidget_set_fullscreen(TerminalWidget * widget,
		gboolean fullscreen)
{
	TerminalWindow * window;

#ifndef EMBEDDED
	if(fullscreen)
		gtk_widget_hide(widget->menubar);
	else
		gtk_widget_show(widget->menubar);
#endif
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(
				widget->fullscreen), fullscreen);
	window = terminal_get_window(widget->terminal);
	terminalwindow_set_fullscreen(window, fullscreen);
}


/* useful */
/* interface */
/* terminalwidget_show_about */
static gboolean _show_about_on_closex(gpointer data);

void terminalwidget_show_about(TerminalWidget * widget, gboolean show)
{
	TerminalWindow * twindow;
	GtkWidget * window;

	if(widget->about == NULL)
	{
		twindow = terminal_get_window(widget->terminal);
		window = terminalwindow_get_window(twindow);
		widget->about = desktop_about_dialog_new();
		gtk_window_set_modal(GTK_WINDOW(widget->about), TRUE);
		gtk_window_set_transient_for(GTK_WINDOW(widget->about),
				GTK_WINDOW(window));
		desktop_about_dialog_set_authors(widget->about, _authors);
		desktop_about_dialog_set_comments(widget->about,
				_("Terminal for the DeforaOS desktop"));
		desktop_about_dialog_set_copyright(widget->about, _copyright);
		desktop_about_dialog_set_license(widget->about, _license);
		desktop_about_dialog_set_logo_icon_name(widget->about,
				"terminal");
		desktop_about_dialog_set_program_name(widget->about, PACKAGE);
		desktop_about_dialog_set_version(widget->about, VERSION);
		desktop_about_dialog_set_website(widget->about,
				"https://www.defora.org/");
		g_signal_connect_swapped(widget->about, "delete-event",
				G_CALLBACK(_show_about_on_closex), widget);
	}
	if(show)
		gtk_widget_show(widget->about);
	else
		gtk_widget_hide(widget->about);
}

static gboolean _show_about_on_closex(gpointer data)
{
	TerminalWidget * widget = data;

	gtk_widget_hide(widget->about);
	return TRUE;
}


/* terminalwidget_show_preferences */
void terminalwidget_show_preferences(TerminalWidget * terminal, gboolean show)
{
	/* FIXME implement */
}


/* terminalwidget_tab_close */
void terminalwidget_tab_close(TerminalWidget * widget, TerminalTab * tab)
{
	size_t i;

	for(i = 0; i < widget->tabs_cnt; i++)
		if(widget->tabs[i] == tab)
		{
			_terminalwidget_tab_close(widget, i);
			return;
		}
}


/* terminalwidget_tab_close_all */
static gboolean _tab_close_all_confirm(TerminalWidget * widget);

void terminalwidget_tab_close_all(TerminalWidget * widget)
{
	if(widget->tabs_cnt > 1 && _tab_close_all_confirm(widget) != TRUE)
		return;
	while(widget->tabs_cnt > 0)
		_terminalwidget_tab_close(widget, widget->tabs_cnt - 1);
}

static gboolean _tab_close_all_confirm(TerminalWidget * widget)
{
	TerminalWindow * twindow;
	GtkWidget * window;
	GtkWidget * dialog;
	gboolean res;

	twindow = terminal_get_window(widget->terminal);
	window = terminalwindow_get_window(twindow);
	dialog = gtk_message_dialog_new(GTK_WINDOW(window),
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


/* terminalwidget_tab_open */
TerminalTab * terminalwidget_tab_open(TerminalWidget * widget)
{
	TerminalTab ** p;
	TerminalTab * tab;
	GtkWidget * label;
	GtkWidget * child;

	if((p = realloc(widget->tabs, sizeof(*p) * (widget->tabs_cnt + 1)))
			== NULL)
		return NULL;
	widget->tabs = p;
	if((tab = terminaltab_new(widget->terminal)) == NULL)
		return NULL;
	widget->tabs[widget->tabs_cnt++] = tab;
	label = terminaltab_get_label(tab);
	child = terminaltab_get_widget(tab);
	gtk_notebook_append_page(GTK_NOTEBOOK(widget->notebook), child, label);
#if GTK_CHECK_VERSION(2, 10, 0)
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(widget->notebook), child,
			TRUE);
#endif
	return tab;
}


/* private */
/* terminalwidget_tab_close */
static void _terminalwidget_tab_close(TerminalWidget * widget, unsigned int i)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, i);
#endif
	if(i >= widget->tabs_cnt)
		return;
	terminaltab_delete(widget->tabs[i]);
	gtk_notebook_remove_page(GTK_NOTEBOOK(widget->notebook), i);
	memmove(&widget->tabs[i], &widget->tabs[i + 1],
			(widget->tabs_cnt - (i + 1)) * sizeof(*widget->tabs));
	if(--widget->tabs_cnt == 0)
		/* XXX there might be multiple windows opened */
		gtk_main_quit();
}


/* callbacks */
/*_terminalwidget_on_close */
static void _terminalwidget_on_close(gpointer data)
{
	TerminalWidget * widget = data;
	int i;

	i = gtk_notebook_get_current_page(GTK_NOTEBOOK(widget->notebook));
	if(i < 0)
		return;
	_terminalwidget_tab_close(widget, i);
}


/*_terminalwidget_on_fullscreen */
static void _terminalwidget_on_fullscreen(gpointer data)
{
	TerminalWidget * widget = data;
	TerminalWindow * window;

	window = terminal_get_window(widget->terminal);
	terminalwindow_set_fullscreen(window,
			terminalwindow_get_fullscreen(window) ? FALSE : TRUE);
}


/*_terminalwidget_on_new_tab */
static void _terminalwidget_on_new_tab(gpointer data)
{
	TerminalWidget * widget = data;

	terminalwidget_tab_open(widget);
}


/*_terminalwidget_on_new_window */
static void _terminalwidget_on_new_window(gpointer data)
{
	TerminalWidget * widget = data;

	/* XXX may fail */
	terminal_window_open(widget->terminal);
}


#ifndef EMBEDDED
/*_terminalwidget_on_file_close */
static void _terminalwidget_on_file_close(gpointer data)
{
	TerminalWidget * widget = data;

	_terminalwidget_on_close(widget);
}


/*_terminalwidget_on_file_close_all */
static void _terminalwidget_on_file_close_all(gpointer data)
{
	TerminalWidget * widget = data;

	terminalwidget_tab_close_all(widget);
}


/*_terminalwidget_on_file_new_tab */
static void _terminalwidget_on_file_new_tab(gpointer data)
{
	TerminalWidget * widget = data;

	terminalwidget_tab_open(widget);
}


/*_terminalwidget_on_file_new_window */
static void _terminalwidget_on_file_new_window(gpointer data)
{
	TerminalWidget * widget = data;

	terminal_window_open(widget->terminal);
}


/*_terminalwidget_on_help_about */
static void _terminalwidget_on_help_about(gpointer data)
{
	TerminalWidget * widget = data;

	terminalwidget_show_about(widget, TRUE);
}


/*_terminalwidget_on_help_contents */
static void _terminalwidget_on_help_contents(gpointer data)
{
	(void) data;

	desktop_help_contents(PACKAGE, PROGNAME_TERMINAL);
}


/*_terminalwidget_on_view_fullscreen */
static void _terminalwidget_on_view_fullscreen(gpointer data)
{
	TerminalWidget * widget = data;

	_terminalwidget_on_fullscreen(widget);
}
#endif
