#ifndef GTERM_H
#define GTERM_H
	
#include "gtw_window.h"
#include <unistd.h>
#include "gte_terminal.h"

/* Make a new GTerm window */
void new_window();

/* Close a GTerm window */
void close_window(GtwWindow* term);

/* Add a new tab to a window */
void new_tab(GtwWindow* term);

#endif /* GTERM_H */
