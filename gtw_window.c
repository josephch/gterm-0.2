#include "gtw_window.h"

GtwWindow* gtw_window_new(){
	GtwWindow* new_window = (GtwWindow*)(g_malloc(sizeof(GtwWindow)));

	/* Create and configure the top-level window */
	new_window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(new_window->window), "Default Window");
	gtk_window_set_default_size(GTK_WINDOW(new_window->window), 400, 300);
	gtk_container_set_border_width(GTK_CONTAINER(new_window->window), 0);

	/* Set up window contents */
	new_window->vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(new_window->window), new_window->vbox);
	gtk_container_set_border_width(GTK_CONTAINER(new_window->vbox), 0);
	
	new_window->notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(new_window->notebook), TRUE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(new_window->notebook), FALSE);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(new_window->notebook), FALSE);

	new_window->statusbar = gtk_statusbar_new();

	/* Temporary blank menubar */
	new_window->menubar = gtk_menu_bar_new();

	gtk_box_pack_start(GTK_BOX(new_window->vbox), new_window->menubar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(new_window->vbox), new_window->notebook);

	/* Why so much RAM used by this? */
	gtk_box_pack_start(GTK_BOX(new_window->vbox), new_window->statusbar, FALSE, FALSE, 0);

	/* Blank pre title by default */
	new_window->pre_title_text = g_strdup("");

	/* No tabs yet */
	new_window->active_tab = NULL;
	new_window->tab_count = 0;

	/* Make room for tabs */
	new_window->tab_array = g_ptr_array_sized_new(1);

	return new_window;
}

void gtw_window_free(GtwWindow* window){
	/*** TABS SHOULD BE CLEARED OUTSIDE OF THIS FUNCTION ***/

	/* Clean up GTK+ widgets */
	gtk_widget_destroy(window->window);

	/* Remove the window from memory */
	g_free(window->pre_title_text);
	g_ptr_array_free(window->tab_array, FALSE);
	g_free(window);
}

void gtw_window_add_tab(GtwWindow* window, GtwTab* tab){
	if (window == NULL){
		g_print("Can't add a tab to a NULL window\n");
	}

	if (tab == NULL){
		g_print("Can't add a NULL tab to a window\n");
	}

	/* Set tab parent */
	tab->parent = window;

	/* Add the tab to the window */
	g_ptr_array_add(window->tab_array, tab);
	gtk_notebook_append_page(GTK_NOTEBOOK(window->notebook), tab->child, NULL);

	/* Increment tab count */
	window->tab_count++;

	gtk_widget_show_all(window->notebook);

	/* Unhide the tab bar if there is more than 1 tab open */
	if (window->tab_count > 1){
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(window->notebook), TRUE);
	}

	/* Set current term to the new terminal tab */
	window->active_tab = tab;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->notebook), -1);

	/* Attach focus to the new tab */
	gtk_widget_grab_focus(tab->child);
}

void gtw_window_remove_tab(GtwWindow* window, GtwTab* tab){
	/* Find the index of the tab to remove */
	int index = gtk_notebook_page_num(GTK_NOTEBOOK(window->notebook), tab->child);

	/* Remove it from the notebook */
	gtk_notebook_remove_page(GTK_NOTEBOOK(window->notebook), index);

	/* Remove it from the tab array */
	g_ptr_array_remove_index(window->tab_array, index);

	/* Decrement tab count */
	window->tab_count--;

	/* Hide the tab bar if no more than 1 tab exists */
	if (window->tab_count <= 1){
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(window->notebook), FALSE);
	}
}

void gtw_window_set_title(GtwWindow* window, gchar* title){
	/* The one string function ... to RULE THEM ALL */
	gchar* new_title = g_strconcat(window->pre_title_text, title, NULL);
	gtk_window_set_title(GTK_WINDOW(window->window), new_title);
	g_free(new_title);
}

void gtw_window_refresh_tab_title(GtwWindow* window, GtwTab* tab){
	gchar* text;

	if (tab->user_title_set){
		text = g_strdup(tab->user_title);
	}

	else {
		text = g_strdup(tab->title);
	}

	/* If title of currently active tab changed, change whole window title */
	if (tab == window->active_tab){
		gtw_window_set_title(window, text);
	}

	int max_tab_title_length = 21; /* In future, will provide way to change this number externally. For now, set it here */
	if (g_utf8_strlen(text, -1) > max_tab_title_length){
		char* buffer = (char*)(g_malloc(max_tab_title_length));
		g_strlcpy(buffer, text, max_tab_title_length - 3);
		buffer[max_tab_title_length - 4] = '.';
		buffer[max_tab_title_length - 3] = '.';
		buffer[max_tab_title_length - 2] = '.';
		buffer[max_tab_title_length - 1] = 0;
		gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(window->notebook), tab->child, buffer);
		g_free(buffer);
	}

	else {
		gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(window->notebook), tab->child, text);
	}

	g_free(text);
}

void gtw_window_set_pre_title_text(GtwWindow* window, gchar* text){
	g_free(window->pre_title_text);
	window->pre_title_text = g_strdup(text);
}

GtwTab* gtw_window_get_nth_tab(GtwWindow* window, guint index){
	return (GtwTab*)(g_ptr_array_index(window->tab_array, index));
}
