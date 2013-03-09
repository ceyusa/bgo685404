#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "xwindow.h"
#include "xvwindow.h"

struct xdata {
	GtkWidget *widget;
	Display *display;
	Window window;
	GC gc;
	int x;
	int y;
	int ww;
	int wh;
	int iw;
	int ih;
};

guint test_handler = 0;

static void
test(bool xv, struct xdata *data)
{
	XWindow *win;

	if (xv)
		win = new XVWindow();
	else
		win = new XWindow();

	Display *display = XOpenDisplay(0);
	g_printerr("opened display %p\n", display);

	if (win->Init(display,
		      data->window,
		      data->gc,
		      data->x,
		      data->y,
		      data->ww,
		      data->wh,
		      data->iw,
		      data->ih))
		g_printerr("%swin init ok!\n", xv ? "xv" : "x");
	else
		g_printerr("%swin init failed\n", xv ? "xv" : "x");

	delete win;

	XCloseDisplay(display);
}

static gboolean
run_tests(gpointer data)
{
	struct xdata *xdata = static_cast<struct xdata*>(data);
	static bool xv = false;

	test(xv, xdata);
	xv = !xv;

	return TRUE;
}

static gboolean
exposed(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	struct xdata *xdata = static_cast<struct xdata*>(data);
	GtkAllocation alloc;

	xdata->display = GDK_WINDOW_XDISPLAY(gtk_widget_get_window(xdata->widget));
	xdata->window = GDK_WINDOW_XID(gtk_widget_get_window(xdata->widget));

	if (!xdata->gc)
		xdata->gc = XCreateGC(xdata->display, xdata->window, 0, NULL);

	gtk_widget_get_allocation(xdata->widget, &alloc);
	xdata->x = alloc.x;
	xdata->y = alloc.y;
	xdata->ww = alloc.width;
	xdata->wh = alloc.height;
	xdata->iw = alloc.width;
	xdata->ih = alloc.height;

	g_printerr("widget display %p / %p\n",
		   GDK_WINDOW_XDISPLAY(gtk_widget_get_window(widget)),
		   GDK_SCREEN_XDISPLAY(gtk_widget_get_screen(widget)));

	if (test_handler == 0)
		test_handler = g_timeout_add_seconds(2, run_tests, xdata);

	return FALSE;
}

static gboolean
deleted(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	struct xdata *xdata = static_cast<struct xdata*>(data);

	if (xdata->gc && xdata->display)
		XFreeGC(xdata->display, xdata->gc);

	return FALSE;
}

static void
quit(GtkWidget *widget, gpointer data)
{
	if (test_handler > 0) {
		g_source_remove(test_handler);
		test_handler = G_MAXUINT;
	}

	gtk_main_quit();
}

int main(int argc, char **argv)
{
	GtkWidget *window, *button, *image, *box;
	struct xdata xdata;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	xdata.widget = image = gtk_image_new();
	box = gtk_vbox_new(FALSE, 3);

	gtk_container_add(GTK_CONTAINER(window), box);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(box), image, TRUE, TRUE, 0);

	g_signal_connect(window, "expose-event", G_CALLBACK(exposed), &xdata);
	g_signal_connect(image, "delete-event", G_CALLBACK(deleted), &xdata);
	g_signal_connect(button, "clicked", G_CALLBACK(quit), &xdata);

	gtk_widget_show_all(window);

	gtk_main ();

	return 0;
}
