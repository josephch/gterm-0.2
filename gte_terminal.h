#ifndef GTE_TERMINAL_H
#define GTE_TERMINAL_H

#include <gtk/gtk.h>

#ifdef USE_VTE_TERMINAL
	#include <vte/vte.h>

	#define GTE VTE
	#define Gte Vte
	#define gte_ vte_
#endif

/* Define stuff */
#define GTE_TERMINAL_TYPE		(gte_terminal_get_type())
#define GTE_TERMINAL(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), GTE_TERMINAL_TYPE, GteTerminal))
#define GTE_TERMINAL_CLASS(kclass)	(G_TYPE_CHECK_CLASS_CAST((kclass), GTE_TERMINAL_TYPE, GteTerminal))
#define IS_GTE_TERMINAL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GTE_TERMINAL_TYPE))
#define IS_GTE_TERMINAL_CLASS(kclass)	(G_TYPE_CHECK_CLASS_TYPE((kclass), GTE_TERMINAL_TYPE))

typedef struct {
	#ifdef USE_VTE_TERMINAL
		VteTerminal widget;
	#else
		GtkWidget widget;
	#endif

	/* For scrolling the terminal */
	GtkAdjustment* adjustment;

	/* Background color */
	GdkColor background;

	/* Foreground */
	GdkColor foreground;

	GtkUpdateType policy;

	char* title;

	glong char_width, char_height;
} GteTerminal;

typedef enum {
        GTE_ERASE_AUTO,
        GTE_ERASE_ASCII_BACKSPACE,
        GTE_ERASE_ASCII_DELETE,
        GTE_ERASE_DELETE_SEQUENCE
} GteTerminalEraseBinding;

typedef enum {
        GTE_ANTI_ALIAS_USE_DEFAULT,
        GTE_ANTI_ALIAS_FORCE_ENABLE,
        GTE_ANTI_ALIAS_FORCE_DISABLE
} GteTerminalAntiAlias;

/* Return a new instance of the terminal widget */
GtkWidget* gte_terminal_new();

/* Set the font */
void gte_terminal_set_font(GteTerminal* term, const PangoFontDescription* font, GteTerminalAntiAlias aliasing);

/* Allow bold or not */
void gte_terminal_set_allow_bold(GteTerminal* term, gboolean allow_bold);

/* Change the colors */
void gte_terminal_set_colors(GteTerminal* terminal, const GdkColor* fg, const GdkColor* bg, const GdkColor* palette, glong palette_size);

/* Set the number of scrollback lines */
void gte_terminal_set_scrollback_lines(GteTerminal* term, glong lines);

/* Enable/Disable transparency */
void gte_terminal_set_transparent(GteTerminal* term, gboolean transparency);

/* Spawn a new command */
pid_t gte_terminal_fork_command(GteTerminal* term, const char *command, char **argv, char **envv, const char *directory, gboolean lastlog, gboolean utmp, gboolean wtmp);

/* Workaround for crappy programs. Hopefully will be removed in the future */
void gte_terminal_set_backspace_binding(GteTerminal* term, GteTerminalEraseBinding binding);

void gte_terminal_get_padding(GteTerminal* term, int* x, int* y);

typedef struct {
	GtkWidgetClass parent_class;
} GteTerminalClass;

/* Something I don't really understand */
GtkType gte_terminal_get_type();

#endif	/* GTE_TERMINAL_H */
