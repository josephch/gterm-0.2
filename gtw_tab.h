#ifndef GTW_TAB_H
#define GTW_TAB_H
	
#include <gtk/gtk.h>

typedef struct {
	/* Holds whatever the user wants */
	GtkWidget*		child;

	/* Title of the tab (duh) */
	gchar*			title;

	/* Pointer to the parent terminal */
	struct _GtwWindow*	parent;

	/* True if tab title was handset by the user, and should not be automatically reset */
	gboolean		user_title_set;

	/* If the user has specified a title, use this string */
	gchar*			user_title;
} GtwTab;

/* Set up a new tab */
GtwTab* gtw_tab_new(GtkWidget* child);

/* Clean up a tab */
void gtw_tab_free(GtwTab* tab);

/* Set the tab title */
void gtw_tab_set_title(GtwTab* tab, gchar* title);

/* Set the user-specified title */
void gtw_tab_set_user_title(GtwTab* tab, gchar* title);

/* Reset the title to default */
void gtw_tab_reset_title(GtwTab* tab);

/* Set the child of the tab */
void gtw_tab_set_child(GtwTab* tab, GtkWidget* child);

#endif /* GTW_TAB_H */
