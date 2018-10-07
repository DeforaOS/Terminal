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



#ifndef TERMINAL_WIDGET_H
# define TERMINAL_WIDGET_H

# include "common.h"


/* TerminalWidget */
/* public */
/* functions */
TerminalWidget * terminalwidget_new(Terminal * terminal, GtkWidget * window);
void terminalwidget_delete(TerminalWidget * widget);


/* accessors */
gboolean terminalwidget_get_fullscreen(TerminalWidget * widget);
void terminalwidget_set_fullscreen(TerminalWidget * widget,
		gboolean fullscreen);

GtkWidget * terminalwidget_get_widget(TerminalWidget * widget);


/* useful */
/* interface */
void terminalwidget_show_about(TerminalWidget * widget, gboolean show);
void terminalwidget_show_preferences(TerminalWidget * widget, gboolean show);

void terminalwidget_tab_close(TerminalWidget * widget, TerminalTab * tab);
void terminalwidget_tab_close_all(TerminalWidget * widget);
TerminalTab * terminalwidget_tab_open(TerminalWidget * widget);

#endif /* !TERMINAL_WIDGET_H */
