#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "xwindow.h"
#include "xvwindow.h"

struct xdata {
	XWindow *xwin;
	Display *xdisp;
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

const char* formats[] = {
	"GBR", "GBR_10LE", "GBR_10BE",
	"RGBx", "BGRx", "xRGB", "xBGR", "RGBA", "BGRA", "ARGB",
	"ABGR", "RGB", "BGR", "RGB16", "BGR16", "RGB15", "BGR15"
};

guint test_handler = 0;
uint8_t *frame = NULL;
bool fs = false;

static bool
create_xwindow(bool xv, struct xdata *data)
{
	if (data->xwin)
		return false;

	if (xv)
		data->xwin = new XVWindow();
	else
		data->xwin = new XWindow();

	data->xdisp = XOpenDisplay(0);
	g_printerr("opened display %p\n", data->xdisp);

	XWindow *win = data->xwin;

	return win->Init(data->xdisp,
			 data->window,
			 data->gc,
			 data->x,
			 data->y,
			 data->ww,
			 data->wh,
			 data->iw,
			 data->ih);
}

static void
destroy_xwindow(struct xdata *data)
{
	if (data->xwin) {
		delete data->xwin;
		data->xwin = 0;

		XCloseDisplay(data->xdisp);
	}
}

static gboolean run_tests(gpointer data);
static void load_frame(const char *filename);

static gboolean
test(gpointer data)
{
	struct xdata *xdata = static_cast<struct xdata*>(data);
	XWindow *win = xdata->xwin;
	static guint c = 0;

	win->ProcessEvents();

	if (frame) {
		g_printerr(".");
		win->PutFrame(frame, 320, 240);
	}

	if (c++ <= 150)
		return TRUE;
	else {
		g_printerr("\n");
		c = 0;
		test_handler = 0;
		destroy_xwindow(xdata);
		return FALSE;
	}
}

static gboolean
run_tests(gpointer data)
{
	struct xdata *xdata = static_cast<struct xdata*>(data);
	static bool xv = false;
	static int i = 0;

	char *filename = g_strconcat(formats[i++ % G_N_ELEMENTS(formats)],
				     ".rgb", NULL);
	load_frame(filename);

	if (frame)
		g_printerr("frame loaded: %s\n", filename);

	g_free(filename);

	if (create_xwindow(xv, xdata)) {
		xv = !xv;
		g_timeout_add(33, test, xdata);
		return FALSE;
	}

	g_printerr("xwin failed\n");
	destroy_xwindow(xdata);
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
	xdata->iw = 320;
	xdata->ih = 240;

	g_printerr("widget display %p / %p\n",
		   GDK_WINDOW_XDISPLAY(gtk_widget_get_window(widget)),
		   GDK_SCREEN_XDISPLAY(gtk_widget_get_screen(widget)));

	if (test_handler == 0)
		test_handler = g_timeout_add_seconds(1, run_tests, xdata);

	return FALSE;
}

static gboolean
deleted(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	struct xdata *xdata = static_cast<struct xdata*>(data);

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

	if (frame)
		g_free(frame);

	gtk_main_quit();
}

static void
fullscreen(GtkWidget *widget, gpointer data)
{
	struct xdata *xdata = static_cast<struct xdata*>(data);

	XWindow *win = xdata->xwin;
	if (!win)
		return;

	fs = !fs;
	destroy_xwindow(xdata);

	if (fs) {
		xdata->window = DefaultRootWindow (XOpenDisplay(0));
		xdata->gc = NULL;
		xdata->x = 0;
		xdata->y = 0;
	}

	if (create_xwindow(true, xdata))
		xdata->xwin->ToggleFullscreen();
}


static void
load_frame(const char *filename)
{
	if (frame)
		g_free(frame);

	if (!g_file_get_contents(filename, (gchar **) &frame, NULL, NULL))
		g_printerr("Can't read frame file: %s", filename);
}

int main(int argc, char **argv)
{
	GtkWidget *window, *button, *image, *box, *fs;
	struct xdata xdata;

	memset(&xdata, 0, sizeof(xdata));

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	fs = gtk_button_new_from_stock(GTK_STOCK_FULLSCREEN);
	xdata.widget = image = gtk_image_new();

#ifdef GTK3
	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
#else
	box = gtk_vbox_new(FALSE, 3);
#endif

	gtk_container_add(GTK_CONTAINER(window), box);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), fs, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(box), image, TRUE, TRUE, 0);

#ifdef GTK3
	g_signal_connect(window, "event", G_CALLBACK(exposed), &xdata);
#else
	g_signal_connect(window, "expose-event", G_CALLBACK(exposed), &xdata);
#endif

	g_signal_connect(image, "delete-event", G_CALLBACK(deleted), &xdata);
	g_signal_connect(button, "clicked", G_CALLBACK(quit), &xdata);
	g_signal_connect(fs, "clicked", G_CALLBACK(fullscreen), &xdata);

	gtk_widget_show_all(window);

	gtk_main ();

	return 0;
}
