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



#ifndef TERMINAL_WINDOW_H
# define TERMINAL_WINDOW_H

# include "common.h"


/* TerminalWindow */
/* public */
/* functions */
TerminalWindow * terminalwindow_new(Terminal * terminal);
void terminalwindow_delete(TerminalWindow * window);


/* accessors */
gboolean terminalwindow_get_fullscreen(TerminalWindow * window);
void terminalwindow_set_fullscreen(TerminalWindow * window,
		gboolean fullscreen);

TerminalWidget * terminalwindow_get_widget(TerminalWindow * window);
GtkWidget * terminalwindow_get_window(TerminalWindow * window);

/* useful */
/* interface */
void terminalwindow_show_about(TerminalWindow * terminal, gboolean show);
void terminalwindow_show_preferences(TerminalWindow * terminal, gboolean show);

#endif /* !TERMINAL_WINDOW_H */
