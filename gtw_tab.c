#include "gtw_tab.h"

GtwTab* gtw_tab_new(GtkWidget* child){
	GtwTab* tab = (GtwTab*)(g_malloc(sizeof(GtwTab)));

	/* No parent yet */
	tab->parent = NULL;

	/* Use default tab title */
	tab->title = g_strdup("No Tab Title");
	tab->user_title_set = FALSE;
	tab->user_title = g_strdup("");

	/* Use user-set contents */
	tab->child = child;

	return tab;
}


void gtw_tab_free(GtwTab* tab){
	g_free(tab->title);
	g_free(tab->user_title);
	g_free(tab);
}

void gtw_tab_set_title(GtwTab* tab, gchar* title){
	g_free(tab->title);
	tab->title = g_strdup(title);
}

void gtw_tab_set_user_title(GtwTab* tab, gchar* title){
	g_free(tab->user_title);
	tab->user_title = g_strdup(title);
	tab->user_title_set = TRUE;
}

void gtw_tab_reset_title(GtwTab* tab){
	g_free(tab->user_title);
	tab->user_title = g_strdup("");
	tab->user_title_set = FALSE;
}

void gtw_tab_set_child(GtwTab* tab, GtkWidget* child){
	gtk_widget_destroy(tab->child);
	tab->child = child;
}
