#ifndef GTW_WINDOW_H
#define GTW_WINDOW_H
	
#include "gtw_tab.h"

typedef struct _GtwWindow {
	/* Top-level window */
	GtkWidget*		window;

	/* Sole child of the window, holds all the bits together */
	GtkWidget*		vbox;

	/* Holds all the tabs */
	GtkWidget*		notebook;

	/* List of current tabs */
	GPtrArray*		tab_array;

	/* Current tab */
	GtwTab*			active_tab;

	/* The main menu bar */	
	GtkWidget*		menubar;

	/* The statusbar */
	GtkWidget*		statusbar;

	/* This is prepended to any titles set with gtw_window_set_title(). Set to "" to disable */
	gchar*			pre_title_text;

	/* Number of tabs in the window */
	gint			tab_count;
} GtwWindow;

/* Initialize the window */
GtwWindow* gtw_window_new();

/* Clean up the window */
void gtw_window_free(GtwWindow* window);

/* Add a tab to the window */
void gtw_window_add_tab(GtwWindow* window, GtwTab* tab);

/* Remove a tab from the window */
void gtw_window_remove_tab(GtwWindow* window, GtwTab* tab);

/* Change the window title */
void gtw_window_set_title(GtwWindow* window, gchar* title);

/* Update the title of a tab */
void gtw_window_refresh_tab_title(GtwWindow* window, GtwTab* tab);

/* Set the text prepended to every title */
void gtw_window_set_pre_title_text(GtwWindow* window, gchar* text);

/* Return the number of tabs in the window */
guint gtw_window_get_n_tabs(GtwWindow* window);

/* Return the n-th tab */
GtwTab* gtw_window_get_nth_tab(GtwWindow* window, guint index);

#endif /* GTERMWINDOW_H */
