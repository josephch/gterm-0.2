#include "gte_terminal.h"

/* Right now, GteTerminal is really just an abstraction of VTE.
 * So, most of this will be #ifdef'ed out
 */

#ifndef USE_VTE_TERMINAL

/* Declarations */

static void gte_terminal_class_init(GteTerminalClass* term_class);
static void gte_terminal_class_init(GteTerminalClass* term_class); 
static void gte_terminal_init(GteTerminal* term);
static void gte_terminal_destroy (GtkObject* object); 
static void gte_terminal_realize (GtkWidget* widget);
static void gte_terminal_size_request (GtkWidget* widget, GtkRequisition* requisition);
static void gte_terminal_size_allocate (GtkWidget* widget, GtkAllocation* allocation);
static gboolean gte_terminal_expose( GtkWidget *widget, GdkEventExpose *event );

static GtkWidgetClass *parent_class = NULL;

GtkType gte_terminal_get_type() {
	static GtkType terminal_type = 0;

	if (!terminal_type)
		{
			static const GtkTypeInfo terminal_info =
			{
				"GteTerminal",
				sizeof (GteTerminal),
				sizeof (GteTerminalClass),
				(GtkClassInitFunc) gte_terminal_class_init,
				(GtkObjectInitFunc) gte_terminal_init,
				/* reserved_1 */ NULL,
				/* reserved_1 */ NULL,
				(GtkClassInitFunc) NULL
			};

			terminal_type = gtk_type_unique (GTK_TYPE_WIDGET, &terminal_info);
		}

	return terminal_type;
}

static void
gte_terminal_class_init(GteTerminalClass* term_class){
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*)(term_class);
	widget_class = (GtkWidgetClass*)(term_class);

	parent_class = (GtkWidgetClass*)(gtk_type_class(gtk_widget_get_type()));

	object_class->destroy = gte_terminal_destroy;

	widget_class->realize = gte_terminal_realize;
	widget_class->expose_event = gte_terminal_expose;
	widget_class->size_request = gte_terminal_size_request;
	widget_class->size_allocate = gte_terminal_size_allocate;
/*	widget_class->button_press_event = gte_terminal_button_press;
	widget_class->button_release_event = gte_terminal_button_release;
	widget_class->motion_notify_event = gte_terminal_motion_notify;*/
}

static void
gte_terminal_init(GteTerminal *term)
{
	term->policy = GTK_UPDATE_CONTINUOUS;
	term->adjustment = NULL;
}

GtkWidget* gte_terminal_new(){
	GteTerminal *term;

	term = (GteTerminal*)(gtk_type_new(gte_terminal_get_type()));

/*	if (!adjustment)
		adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	gte_terminal_set_adjustment (term, adjustment);
*/
	term->background.red = term->background.blue = term->background.green = 0xffff;
	term->foreground.red = term->foreground.blue = term->foreground.green = 0x0000;

	return GTK_WIDGET(term);
}

static void
gte_terminal_destroy (GtkObject *object)
{
	GteTerminal *term;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_GTE_TERMINAL(object));

	term = GTE_TERMINAL(object);

/*	if (term->adjustment)
		gtk_object_unref (GTK_OBJECT (term->adjustment));
*/
	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(*GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void
gte_terminal_realize (GtkWidget *widget)
{
	GteTerminal *term;
	GdkWindowAttr attributes;
	gint attributes_mask;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (IS_GTE_TERMINAL (widget));

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	term = GTE_TERMINAL (widget);

	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = gtk_widget_get_events (widget) | 
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
		GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
		GDK_POINTER_MOTION_HINT_MASK;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

	widget->style = gtk_style_attach (widget->style, widget->window);

	gdk_window_set_user_data (widget->window, widget);

	GdkColormap* colormap = gdk_drawable_get_colormap(widget->window);
	gdk_colormap_alloc_color(colormap, &(term->background), FALSE, TRUE);
	gdk_colormap_alloc_color(colormap, &(term->foreground), FALSE, TRUE);

	gdk_window_set_background(widget->window, &(term->background));
}

static void 
gte_terminal_size_request (GtkWidget			*widget,
					 GtkRequisition *requisition)
{
	requisition->width = 100;
	requisition->height = 100;
/*
	requisition->width = DIAL_DEFAULT_SIZE;
	requisition->height = DIAL_DEFAULT_SIZE;
*/
}

static void
gte_terminal_size_allocate (GtkWidget		 *widget,
			GtkAllocation *allocation)
{
	GteTerminal *term;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (IS_GTE_TERMINAL (widget));
	g_return_if_fail (allocation != NULL);

	widget->allocation = *allocation;
	if (GTK_WIDGET_REALIZED (widget))
		{
			term = GTE_TERMINAL (widget);

			gdk_window_move_resize (widget->window,
						allocation->x, allocation->y,
						allocation->width, allocation->height);

/*			term->radius = MAX(allocation->width,allocation->height) * 0.45;
			term->pointer_width = term->radius / 5;
*/
		}
}

static gboolean
gte_terminal_expose( GtkWidget *widget, GdkEventExpose *event )
{
	GteTerminal *term;
/*	GdkPoint points[3];
	gdouble s,c;
	gdouble theta;
	gint xc, yc;
	gint tick_length;
	gint i; */

	g_return_val_if_fail (widget != NULL, FALSE);
	g_return_val_if_fail (IS_GTE_TERMINAL (widget), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (event->count > 0)
		return FALSE;
	
	term = GTE_TERMINAL (widget);

/*	xc = widget->allocation.width/2;
	yc = widget->allocation.height/2;
*/
	/* Draw ticks */

/*	for (i=0; i<25; i++)
		{
			theta = (i*M_PI/18. - M_PI/6.);
			s = sin(theta);
			c = cos(theta);

			tick_length = (i%6 == 0) ? term->pointer_width : term->pointer_width/2;
			
			gdk_draw_line (widget->window,
				 widget->style->fg_gc[widget->state],
				 xc + c*(term->radius - tick_length),
				 yc - s*(term->radius - tick_length),
				 xc + c*term->radius,
				 yc - s*term->radius);
		}
*/
	/* Draw pointer */

/*	s = sin(term->angle);
	c = cos(term->angle);


	points[0].x = xc + s*term->pointer_width/2;
	points[0].y = yc + c*term->pointer_width/2;
	points[1].x = xc + c*term->radius;
	points[1].y = yc - s*term->radius;
	points[2].x = xc - s*term->pointer_width/2;
	points[2].y = yc - c*term->pointer_width/2;

	gtk_draw_polygon (widget->style,
				widget->window,
				GTK_STATE_NORMAL,
				GTK_SHADOW_OUT,
				points, 3,
				TRUE);*/
	gdk_window_clear(widget->window);
	gdk_draw_line(widget->window, widget->style->fg_gc[widget->state],
		0, 0, 10, 10);
	return FALSE;
}

void gte_terminal_set_colors(GteTerminal* term, GdkColor* foreground, GdkColor* background, const GdkColor *palette, glong palette_size){
	g_return_if_fail(IS_GTE_TERMINAL(term));

	if (foreground != NULL){
		term->foreground = *foreground;
		if (GTK_WIDGET_REALIZED(term)){
			GdkDrawable* drawable = GDK_DRAWABLE(GTK_WIDGET(term)->window);
			GdkColormap* colormap = gdk_drawable_get_colormap(drawable);
			gdk_colormap_free_colors(colormap, &(term->foreground), 1);
			gdk_colormap_alloc_color(colormap, &(term->foreground), FALSE, TRUE);
		}
	}

	if (background != NULL){
		term->background = *background;
		if (GTK_WIDGET_REALIZED(term)){
			GdkDrawable* drawable = GDK_DRAWABLE(GTK_WIDGET(term)->window);
			GdkColormap* colormap = gdk_drawable_get_colormap(drawable);
			gdk_colormap_free_colors(colormap, &(term->background), 1);
			gdk_colormap_alloc_color(colormap, &(term->background), FALSE, TRUE);
			gdk_window_set_background(GTK_WIDGET(term)->window, &(term->background));
		}
	}
}
#endif /* No USE_VTE_TERMINAL */
