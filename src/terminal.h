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



#ifndef TERMINAL_TERMINAL_H
# define TERMINAL_TERMINAL_H

# include "common.h"


/* Terminal */
/* public */
/* types */
typedef struct _TerminalPrefs
{
	String const * shell;
	String const * directory;
	unsigned int login;
} TerminalPrefs;


/* functions */
/* essential */
Terminal * terminal_new(TerminalPrefs * prefs);
void terminal_delete(Terminal * terminal);

/* accessors */
String const * terminal_get_directory(Terminal * terminal);
String const * terminal_get_shell(Terminal * terminal);
TerminalWindow * terminal_get_window(Terminal * terminal);
gboolean terminal_is_login(Terminal * terminal);

/* useful */
void terminal_tab_close(Terminal * terminal, TerminalTab * tab);
void terminal_tab_close_all(Terminal * terminal);
TerminalTab * terminal_tab_open(Terminal * terminal);

int terminal_window_open(Terminal * terminal);

#endif /* !TERMINAL_TERMINAL_H */
