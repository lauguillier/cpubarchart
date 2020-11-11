#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <cairo.h>
#include <gtk/gtk.h>

#define STR_LEN 100
#define MAX_CPU 32
#define DFLT_BW 20
#define DFLT_BR 1.15
#define DFLT_FS 100
#define DFLT_HR 1.2
#define DFLT_WR 1.4

#define COLOR_BG 0.203, 0.239, 0.274
#define COLOR_BAR 0.4, 0.6, 0.8
#define COLOR_AVG 0.960, 0.682, 0.337
#define COLOR_GRID 0.650, 0.674, 0.725

typedef struct 
{
	// gtk win geometry
	int ww;
	int wh;

	// drawing area
	int w;
	int h;
	float hratio;
	float wratio;
	int x0;
	int y0;

	//bargraph dimensions
	int barwidth;
	float barwidth_ratio;
	int nbcpu;
	int fullscale;

	// Grid
	int zero;
	int cent;
	int fifty;
	int quart;
	int troisquart;

} chart_geometry;

long int sum[MAX_CPU] = {0}, idle[MAX_CPU] = {0}, lastSum[MAX_CPU] = {0}, lastIdle[MAX_CPU] = {0};
int busyFraction[MAX_CPU] = {0};

char *str;
chart_geometry *cgeo;


inline int min ( int a, int b ) { return a < b ? a : b; }

void draw_hline(cairo_t *cr, int h)
{
	cairo_move_to (cr, cgeo->x0, h);
	cairo_line_to (cr, cgeo->x0 + cgeo->w, h);
}

void draw_grid(cairo_t *cr)
{

	// Background
	cairo_save(cr);
	cairo_set_source_rgb(cr, COLOR_BG);
	cairo_paint(cr);
	cairo_restore(cr);

	// Horizontal grid lines
	draw_hline(cr, cgeo->zero);
	draw_hline(cr, cgeo->fifty);
	draw_hline(cr, cgeo->cent);
	draw_hline(cr, cgeo->quart);
	draw_hline(cr, cgeo->troisquart);

	cairo_set_source_rgb(cr, COLOR_GRID);
	cairo_set_line_width (cr, 0.5);
	cairo_stroke (cr);
}

void draw_bargraph (cairo_t *cr)
{
	int j, dy;

	// draw grid
	draw_grid(cr);

	// Draw bargraph for each core
	for (j=1; j<=cgeo->nbcpu; j++)
	{
		//printf("CPU%c : %d %% busy\n", '0'+j-1, busyFraction[j]);
		dy = busyFraction[j] ? busyFraction[j]*cgeo->fullscale/100 : 1;
		cairo_rectangle(cr, cgeo->x0+cgeo->barwidth*cgeo->barwidth_ratio*(j-1), cgeo->zero-dy, cgeo->barwidth, dy);
	}
	cairo_set_source_rgb(cr, COLOR_BAR);   /* set fill color */
	cairo_fill(cr);                            /* fill rectangle */

	// Draw an horizontal line for global CPU load
	dy = busyFraction[0]?busyFraction[0]:1;
	cairo_move_to (cr, cgeo->x0, cgeo->zero-dy);
	cairo_line_to (cr, cgeo->x0 + cgeo->w, cgeo->zero-dy);
	cairo_set_source_rgb(cr, COLOR_AVG);
	cairo_set_line_width (cr, 2);
	cairo_stroke (cr);
}

static gboolean on_draw_event (GtkWidget *widget, cairo_t *cr,
							gpointer user_data)
{
	draw_bargraph (cr);     /* draw rectangle in window */

	return FALSE;
	(void)user_data, (void)widget;  /* suppress -Wunused warning */
}

// process_stat() performs the following :
// - get the (nbcpu+1) first lines in /proc/stat pseudo file,
// - compute each core activity by comparing idle field evolution since the last call
// and the total tick evolution since the last call
// - obviously, first call results are irrelevant
// - Fisrt line is the average for the whole CPU
// - next nbcpu lines read the cores stats

int process_stat(int nbcpu)
{
	
	const char d[2] = " ";
	char* token;
	size_t size;
	int i, j;
	int ret = -1;
	
	FILE* fp = fopen("/proc/stat","r");
	

	for (j=0; j<=nbcpu; j++)
	{
		// get the lines 
		ret = getline(&str, &size, fp);
		if (ret == -1)
		{
			printf("Error getline\n");
			break;
		}
		//printf("getline : %s", str);
		
		token = strtok(str,d);
		sum[j] = 0;
		i = 0;
		// Get all the numerical fields in the current line
		while(token!=NULL)
		{
			token = strtok(NULL,d);
			//printf("\t%s\n", token);
			if(token!=NULL)
			{
				// Total time is the sum of all fields
				sum[j] += atoi(token);
				// idle time is 4th field
				if(i==3)
					idle[j] = atoi(token);

				i++;
			}
		}

		// If this is the first call, prepare to return 0 to tell the results are irrelevant
		ret = lastSum[0]?1:0;
		// Compute the activity of each core plus mean and store 
		busyFraction[j] = (int)(100.0 - (idle[j]-lastIdle[j])*100.0/(sum[j]-lastSum[j]));
		lastIdle[j] = idle[j];
		lastSum[j] = sum[j];
	}
	fclose(fp);
	return(ret);
}

gboolean time_handler(GtkWidget *widget)
{
	if (widget == NULL)
		return FALSE;
	
	process_stat(cgeo->nbcpu);

	gtk_widget_queue_draw(widget);
  
	return TRUE;
}


int main(int argc, char* argv[])
{
	GtkWidget *window;          /* gtk windows */
	GtkWidget *darea;           /* cairo drawing area */

	gtk_init (&argc, &argv);    /* required with every gtk app */

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);  /* create window */
	darea = gtk_drawing_area_new();                 /* create cairo area */
	gtk_container_add (GTK_CONTAINER(window), darea);   /* add to window */

	cgeo = (chart_geometry*)malloc(sizeof(chart_geometry));
	if (cgeo==NULL)
	{
		fprintf(stderr, "Error malloc chart_geometry\n");
		return (-1);
	}

	cgeo->nbcpu = min(get_nprocs(), MAX_CPU);

	// Initial values for geometry
	cgeo->barwidth = DFLT_BW;
	cgeo->barwidth_ratio = DFLT_BR;
	cgeo->hratio = DFLT_HR;
	cgeo->wratio = DFLT_WR;
	cgeo->fullscale = DFLT_FS;
	cgeo->h = cgeo->fullscale;
	cgeo->w = cgeo->nbcpu * cgeo->barwidth + (cgeo->nbcpu-1) * cgeo->barwidth * (cgeo->barwidth_ratio - 1);
	cgeo->x0 = cgeo->w * (cgeo->wratio -1) / 2;
	cgeo->y0 = cgeo->fullscale *(cgeo->hratio - 1) / 2;

	cgeo->cent = cgeo->y0; //(cgeo->h - cgeo->fullscale) / 2;
	cgeo->zero = cgeo->cent + cgeo->fullscale;
	cgeo->fifty = (cgeo->cent + cgeo->zero) / 2;
	cgeo->quart = (cgeo->fifty + cgeo->zero) / 2;
	cgeo->troisquart = (cgeo->fifty + cgeo->cent) / 2;
	cgeo->ww = cgeo->w * cgeo->wratio;
	cgeo->wh = cgeo->h * cgeo->hratio;

	/* connect callbacks to draw rectangle and close window/quit app */
	g_signal_connect (G_OBJECT(darea), "draw",
						G_CALLBACK(on_draw_event), NULL);
	g_signal_connect (G_OBJECT(window), "destroy",
						G_CALLBACK(gtk_main_quit), NULL);

	gtk_window_set_default_size (window, cgeo->ww, cgeo->wh);
	//gtk_window_set_resizable (window, FALSE);

	gtk_window_set_title (window, "CPU Barchart");
	gtk_widget_show_all (window);   /* show all windows */

	printf("This system has %d processors configured and "
	"%d processors available.\n", get_nprocs_conf(), cgeo->nbcpu);
	printf("window ww: %d | wh: %d\n", cgeo->ww, cgeo->wh);
	str = (char *)malloc((size_t)STR_LEN);
	
	g_timeout_add(200, (GSourceFunc) time_handler, (gpointer) window);
	gtk_main();     /* pass control to gtk event-loop */

	free(str);
	free(cgeo);
	return 0;
}
