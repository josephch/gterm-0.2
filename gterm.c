#include "gterm.h"

/* Current number of windows. Exit when this reaches 0 */
static int window_count = 0;

/* Backspace binding for the terminals */

/* Least incompatible. I'll write a termcap entry eventually so braindead programs like 'screen' will work */
static GteTerminalEraseBinding gterm_global_erase_binding = GTE_ERASE_AUTO; /* Uncomment to use */

/* Works with screen */
/* Note that with this, backspace will leave ^H in the terminal when invoked from within a program (try it in something like fdisk). */
/* But the program will detect the keystroke properly */
/* static GteTerminalEraseBinding gterm_global_erase_binding = GTE_ERASE_ASCII_BACKSPACE; */  /* Uncomment to use */

/* Font for the terminals */
static PangoFontDescription* gterm_global_font;

/* If the terminal is transparent or not */
static gboolean gterm_global_transparent;

/* Foreground and background colors */
static GdkColor gterm_global_foreground;
static GdkColor gterm_global_background;

/* Number of scrollback lines */
static glong gterm_global_scrollback_lines;

/* Enable anti-aliasing */
static GteTerminalAntiAlias gterm_global_antialias;

/* General exception handlers */
static gboolean
gterm_window_event_delete_event(GtkWidget* widget, GdkEvent* event, gpointer data){
	close_window((GtwWindow*)(data));
	return TRUE;
}

static void
gterm_menu_event_quit(GtkWidget* widget, gpointer data){
	close_window((GtwWindow*)(data));
}

static void
gterm_menu_event_new_window(GtkWidget* widget, gpointer data){
	new_window();
}

static void
gterm_menu_event_toggle_statusbar(GtkWidget* widget, gpointer data){
	GtwWindow* term = (GtwWindow*)(data);
	gboolean is_visible = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget));

	if (is_visible){
		gtk_widget_show(term->statusbar);
	}

	else {
		gtk_widget_hide(term->statusbar);
	}
}

static void
gterm_menu_event_tab_new(GtkWidget* widget, gpointer data){
	new_tab((GtwWindow*)(data));
}

static void
gterm_menu_event_tab_close(GtkWidget* widget, gpointer data){
	GtwWindow* term = (GtwWindow*)(data);

	/* Don't try to remove a NULL tab */
	if (term->active_tab != NULL){
		gtw_window_remove_tab(term, term->active_tab);
	}

	if (term->tab_count <= 0){
		close_window(term);
	}
}

static void
gterm_menu_event_tab_rename(GtkWidget* widget, gpointer data){
	GtwWindow* term = (GtwWindow*)(data);

	/* Don't rename tabs that don't exist */
	if (term->active_tab == NULL){
		return;
	}

	GtkWidget* dialog_label = gtk_label_new("Enter the tab's new title:");
	GtkWidget* dialog_entry = gtk_entry_new();
	GtkWidget* dialog = gtk_dialog_new_with_buttons("Change tab title", GTK_WINDOW(term->window),
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
		NULL
	);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), dialog_label);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), dialog_entry);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY);
	gtk_widget_show_all(dialog);
	int result = gtk_dialog_run(GTK_DIALOG(dialog));

	switch (result){
		case GTK_RESPONSE_APPLY:{
			/* Find the new title */
			gchar* new_title = g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_entry)));

			/* Set the new title */
			gtw_tab_set_user_title(term->active_tab, new_title);

			/* Tell window to change tab's title */
			gtw_window_refresh_tab_title(term, term->active_tab);

			/* Cleanup in aisle 6 */
			g_free(new_title);
			break;
		}

		default:
		break;
	}

	gtk_widget_destroy(dialog);
}

static void
gterm_menu_event_tab_reset_name(GtkWidget* widget, gpointer data){
	GtwWindow* term = (GtwWindow*)(data);

	/* Don't rename tabs that don't exist */
	if (term->active_tab == NULL){
		return;
	}

	gtw_tab_reset_title(term->active_tab);

	gtw_window_refresh_tab_title(term, term->active_tab);
}

static void
gterm_menu_event_debug_set_backspace(GtkRadioAction* action, GtkRadioAction* current, gpointer data){
	GtwWindow* term = (GtwWindow*)(data);

	/* Which binding? */
	switch (gtk_radio_action_get_current_value(action)){
		case 0:	/* Auto */
			gterm_global_erase_binding = GTE_ERASE_AUTO;
			break;

		case 1:	/* ASCII */
			gterm_global_erase_binding = GTE_ERASE_ASCII_BACKSPACE;
			break;

		default:
			break;
	}

	/* Change the backspace binding for all open tabs */
	int i;
	for (i = 0; i < term->tab_count; i++){
		GList* child_list = gtk_container_get_children(GTK_CONTAINER(gtw_window_get_nth_tab(term, i)->child));
		VteTerminal* terminal_widget = VTE_TERMINAL(child_list->data);
		g_list_free(child_list);

		vte_terminal_set_backspace_binding(VTE_TERMINAL(terminal_widget), gterm_global_erase_binding);
	}
}

static void
gterm_notebook_event_switch_page(GtkWidget* widget, GtkNotebookPage* page, int page_index, gpointer data){
	GtwWindow* term = (GtwWindow*)(data);

	term->active_tab = gtw_window_get_nth_tab(term, page_index);

	/* Refresh window title */
	gtw_window_refresh_tab_title(term, term->active_tab);

	/* Attach focus to the new terminal */
	gtk_widget_grab_focus(term->active_tab->child);
}

/* Exception handlers for the terminal emulators themselves */

static void
gterm_term_event_child_exited(GtkWidget *widget, gpointer data){
	GtwTab* tab = (GtwTab*)(data);
	GtwWindow* parent = tab->parent;

	gtw_window_remove_tab(parent, tab);
	gtw_tab_free(tab);

	if (parent->tab_count <= 0){
		close_window(parent);
	}
}

static void
gterm_term_event_window_title_changed(GtkWidget *widget, gpointer data){
	GtwTab* tab = (GtwTab*)(data);

	GList* child_list = gtk_container_get_children(GTK_CONTAINER(tab->child));
	VteTerminal* terminal_widget = VTE_TERMINAL(child_list->data);
	g_list_free(child_list);

	gchar* title = terminal_widget->window_title;

	gtw_tab_set_title(tab, title);
	gtw_window_refresh_tab_title(tab->parent, tab);
}

/* End exception handlers */

void new_window(){
	/* Create the window */
	GtwWindow* window;
	window = gtw_window_new();

	/* Generate menus */
	gtk_widget_destroy(window->menubar);

	/* Menu creation */
	static GtkActionEntry entries[] = {
		{ "FileMenu", NULL, "_File" },
		{ "TabMenu", NULL, "_Tab" },
		{ "DebugMenu", NULL, "_Debug" },
		{ "NewWindow", GTK_STOCK_NEW, "_New Window", "", "Exit the program", G_CALLBACK(gterm_menu_event_new_window) },
		{ "Exit", GTK_STOCK_QUIT, "_Exit", "", "Exit the program", G_CALLBACK(gterm_menu_event_quit) },
		{ "NewTab", GTK_STOCK_ADD, "_New", "", "Create a new tab", G_CALLBACK(gterm_menu_event_tab_new) },
		{ "CloseTab", GTK_STOCK_REMOVE, "_Close", "", "Close the current tab", G_CALLBACK(gterm_menu_event_tab_close) },
		{ "RenameTab", GTK_STOCK_EDIT, "_Rename", "", "Rename the current tab", G_CALLBACK(gterm_menu_event_tab_rename) },
		{ "ResetTabName", GTK_STOCK_CLEAR, "Re_set Name", "", "Reset the current tab's name", G_CALLBACK(gterm_menu_event_tab_reset_name) }
	};

	static GtkToggleActionEntry toggle_entries[] = {
		{ "ToggleStatus", NULL, "_Toggle statusbar", "", "Turn the statusbar on or off", G_CALLBACK(gterm_menu_event_toggle_statusbar), TRUE }
	};

	static GtkRadioActionEntry radio_entries[] = {
		{ "SetBackspaceBindingAuto", NULL, "Set _Auto Backspace", "", "Set terminal backspace binding to auto", 0 },
		{ "SetBackspaceBindingAscii", NULL, "Set ASCII _Backspace", "", "Set terminal backspace binding to ascii backspace", 1 }
	};

	static const char *ui_description =
		"<ui>"
		"  <menubar name='MainMenu'>"
		"    <menu action='FileMenu'>"
		"      <menuitem action='NewWindow'/>"
		"      <menuitem action='ToggleStatus'/>"
		"      <menuitem action='Exit'/>"
		"    </menu>"
		"    <menu action='TabMenu'>"
		"      <menuitem action='NewTab'/>"
		"      <menuitem action='CloseTab'/>"
		"      <menuitem action='RenameTab'/>"
		"      <menuitem action='ResetTabName'/>"
		"      <separator/>"
		"    </menu>"
		"    <menu action='DebugMenu'>"
		"      <menuitem action='SetBackspaceBindingAuto'/>"
		"      <menuitem action='SetBackspaceBindingAscii'/>"
		"    </menu>"
		"  </menubar>"
		"</ui>";

	/* Ugliest code ever - thank you GTK+ docs */

	GtkActionGroup*	action_group;
	GtkUIManager*	ui_manager;
	GtkAccelGroup*	accel_group;
	GError*		error;

	action_group = gtk_action_group_new("MenuActionGroup");
	gtk_action_group_add_actions(action_group, entries, G_N_ELEMENTS(entries), window);
	gtk_action_group_add_toggle_actions(action_group, toggle_entries, G_N_ELEMENTS(toggle_entries), window);
	gtk_action_group_add_radio_actions(action_group, radio_entries, G_N_ELEMENTS (radio_entries), 1, G_CALLBACK(gterm_menu_event_debug_set_backspace), window);

	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group (ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (window->window), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error)){
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
		exit (EXIT_FAILURE);
	}

	window->menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

	/* End ugly "official" code */

	/* Add the menubar to the window */
	gtk_box_pack_start(GTK_BOX(window->vbox), window->menubar, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(window->vbox), window->menubar, 0);

	/* Connect signal handlers */
	g_signal_connect (G_OBJECT(window->window), "delete_event", G_CALLBACK (gterm_window_event_delete_event), window);
	g_signal_connect (G_OBJECT(window->notebook), "switch-page", G_CALLBACK (gterm_notebook_event_switch_page), window);

	/* Add a tab */
	new_tab(window);

	/* Show the window */
	gtk_widget_show_all(window->window);

	window_count++;

}

void close_window(GtwWindow* term){
	/* Kill all the tabs */
	while (term->tab_count > 0){
		gtw_window_remove_tab(term, gtw_window_get_nth_tab(term, 0));
	}

	gtw_window_free(term);

	window_count--;

	if (window_count <= 0){
		gtk_main_quit();
	}
}

void new_tab(GtwWindow* term){
	/* Create a tab */
	GtwTab* tab;

	/* Create tab contents */
	GtkWidget* terminal_widget = vte_terminal_new();
	GtkWidget* scrolled_window = gtk_hbox_new(FALSE, 0);
	GtkWidget* scrollbar = gtk_vscrollbar_new(VTE_TERMINAL(terminal_widget)->adjustment);
	gtk_box_pack_start(GTK_BOX(scrolled_window), terminal_widget, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(scrolled_window), scrollbar, FALSE, FALSE, 0);

	/* Configure the terminal */

	/* Set the font */
	vte_terminal_set_font_full(VTE_TERMINAL(terminal_widget), gterm_global_font, gterm_global_antialias); 

	/* Bold == fugly, especially the way GTE does it. Comment if you want */
	vte_terminal_set_allow_bold(VTE_TERMINAL(terminal_widget), FALSE); 

	/* Set backspace to whatever */
	vte_terminal_set_backspace_binding(VTE_TERMINAL(terminal_widget), gterm_global_erase_binding); 

	/* Set terminal colors */
	vte_terminal_set_colors(VTE_TERMINAL(terminal_widget), &gterm_global_foreground, &gterm_global_background, NULL, 0);
	vte_terminal_set_colors(VTE_TERMINAL(terminal_widget), &gterm_global_foreground, &gterm_global_background, NULL, 0); 

	/* Number of scrollback lines */
	vte_terminal_set_scrollback_lines(VTE_TERMINAL(terminal_widget), gterm_global_scrollback_lines); 

	/* Let the terminal be transparent */
	vte_terminal_set_background_transparent(VTE_TERMINAL(terminal_widget), gterm_global_transparent);

	/* From the GTE sourcecode. Looks awful, but it fixes problems with resizing */
	GdkGeometry geometry;
	gint xpad, ypad;
	vte_terminal_get_padding(VTE_TERMINAL(terminal_widget), &xpad, &ypad);

	geometry.width_inc = VTE_TERMINAL(terminal_widget)->char_width;
	geometry.height_inc = VTE_TERMINAL(terminal_widget)->char_height;
	geometry.base_width = xpad;
	geometry.base_height = ypad;
	geometry.min_width = xpad + VTE_TERMINAL(terminal_widget)->char_width * 2;
	geometry.min_height = ypad + VTE_TERMINAL(terminal_widget)->char_height * 2;

	gtk_window_set_geometry_hints(GTK_WINDOW(term->window), terminal_widget, &geometry, (GdkWindowHints)(GDK_HINT_RESIZE_INC | GDK_HINT_BASE_SIZE | GDK_HINT_MIN_SIZE) );

	/* Add contents to the tab */
	tab = gtw_tab_new(scrolled_window);

	/* Add the tab to the window */
	gtw_window_add_tab(term, tab);

	/* Take focus */
	gtk_widget_grab_focus(terminal_widget);

	/* Open the shell */
	vte_terminal_fork_command(VTE_TERMINAL(terminal_widget), getenv("SHELL"), NULL, NULL, NULL, FALSE, FALSE, FALSE); 

	/* Connect signal handlers */
	g_signal_connect(G_OBJECT(terminal_widget), "child-exited", G_CALLBACK(gterm_term_event_child_exited), tab);
	g_signal_connect(G_OBJECT(terminal_widget), "window-title-changed", G_CALLBACK(gterm_term_event_window_title_changed), tab);

}

int main(int argc, char** argv){
	gtk_init(&argc, &argv);

	/* Set sensible defaults */

	/* Set easy-to-look-at default colors */
	gterm_global_background.red = gterm_global_background.blue = gterm_global_background.green = 0x0000;
	gterm_global_foreground.red = gterm_global_foreground.blue = gterm_global_foreground.green = 0xffff;

	/* Use default Gtk font */
	gterm_global_font = pango_font_description_new();

	/* Good amount of scrollback lines */
	gterm_global_scrollback_lines = 100;

	/* Parse commandline options */
	static gchar* user_font_string = NULL;
	static gchar* user_background = NULL;
	static gchar* user_foreground = NULL;

	/* Use default setting for aliasing */
	gterm_global_antialias = GTE_ANTI_ALIAS_USE_DEFAULT;
	static gint alias_setting = 1;

	static GOptionEntry option_entries[] = 
	{
		{ "transparent", 't', 0, G_OPTION_ARG_NONE, &gterm_global_transparent, "Make the terminal transparent", NULL },
		{ "antialias", 'a', 0, G_OPTION_ARG_INT, &alias_setting, "Anti-alias terminal fonts. 1 = default, 2 = force enable, 3 = force disable", "SETTING" },
		{ "font", 'f', 0, G_OPTION_ARG_STRING, &user_font_string, "Font to use for the terminals", "\"FONT\"" },
		{ "foreground", 'o', 0, G_OPTION_ARG_STRING, &user_foreground, "Foreground color", "COLOR" },
		{ "background", 'g', 0, G_OPTION_ARG_STRING, &user_background, "Background color", "COLOR" },
		{ "scrollback", 's', 0, G_OPTION_ARG_INT, &gterm_global_scrollback_lines, "Number of scrollback lines", "LINES" },
		{ NULL }
	};

	GError *error = NULL;

	GOptionContext* context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, option_entries, NULL);
	/* This function breaks the program. It was put in the offical documentation as "the way" to do this. Good jorb Cletus, that thar is some good writering! */
/*	g_option_context_add_group (context, gtk_get_option_group (FALSE));*/
	g_option_context_parse (context, &argc, &argv, &error);

	/* Set the font */
	if (user_font_string != NULL){
		pango_font_description_free(gterm_global_font);
		gterm_global_font = pango_font_description_from_string(user_font_string);
	}

	/* Enable or disable anti-aliasing */
	switch (alias_setting){
		case 1:
			gterm_global_antialias = GTE_ANTI_ALIAS_USE_DEFAULT;
			break;

		case 2:
			gterm_global_antialias = GTE_ANTI_ALIAS_FORCE_ENABLE;
			break;

		case 3:
			gterm_global_antialias = GTE_ANTI_ALIAS_FORCE_DISABLE;
			break;
	}

	/* Parse colors */
	if (user_background != NULL){
		gboolean parse_success = gdk_color_parse(user_background, &gterm_global_background);
		if (!parse_success){
			/* If it didn't parse, no color for you! */
			g_print("Unable to parse background \"%s\"\n", user_background);
			gterm_global_background.red = gterm_global_background.blue = gterm_global_background.green = 0x0000;
		}
	}

	if (user_foreground != NULL){
		gboolean parse_success = gdk_color_parse(user_foreground, &gterm_global_foreground);
		if (!parse_success){
			/* If it didn't parse, no color for you! */
			g_print("Unable to parse foreground \"%s\"\n", user_foreground);
			gterm_global_foreground.red = gterm_global_foreground.blue = gterm_global_foreground.green = 0xffff;
		}
	}

	new_window();
	gtk_main();
	pango_font_description_free(gterm_global_font);
	return 0;
}
