#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <cairo.h>
#include <gtk/gtk.h>

#define STR_LEN 100
#define MAX_CPU 32

long int sum[MAX_CPU] = {0}, idle[MAX_CPU] = {0}, lastSum[MAX_CPU] = {0}, lastIdle[MAX_CPU] = {0};
int busyFraction[MAX_CPU] = {0};
int nbcpu;
int barwidth = 20;
char *str;

inline int min ( int a, int b ) { return a < b ? a : b; }

static void draw_bargraph (cairo_t *cr)
{
	int j, dy;


	// Draw bargraph for each processor
	for (j=1; j<=nbcpu; j++)
	{
		//printf("CPU%c : %d %% busy\n", '0'+j-1, busyFraction[j]);
		dy = busyFraction[j]?busyFraction[j]:1;
		cairo_rectangle(cr, barwidth*1.05*j, 110-dy, barwidth, dy);
	}

    cairo_set_source_rgb(cr, 0.3, 0.4, 0.6);   /* set fill color */
    cairo_fill(cr);                            /* fill rectangle */
}

static gboolean on_draw_event (GtkWidget *widget, cairo_t *cr,
                                gpointer user_data)
{
    draw_bargraph (cr);     /* draw rectangle in window */

    return FALSE;
    (void)user_data, (void)widget;  /* suppress -Wunused warning */
}


int process_stat(int nbcpu)
{
	
	const char d[2] = " ";
	char* token;
	size_t size;
	int i, j;
	int ret;
	
	FILE* fp = fopen("/proc/stat","r");
	

	for (j=0; j<=nbcpu; j++)
	{
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
		while(token!=NULL)
		{
			token = strtok(NULL,d);
			//printf("\t%s\n", token);
			if(token!=NULL)
			{
				sum[j] += atoi(token);

				if(i==3)
					idle[j] = atoi(token);

				i++;
			}
		}

		ret = lastSum[0]?1:0;
		
		busyFraction[j] = (int)(100.0 - (idle[j]-lastIdle[j])*100.0/(sum[j]-lastSum[j]));
		lastIdle[j] = idle[j];
		lastSum[j] = sum[j];
	}
	fclose(fp);
	return(ret);
}

gboolean time_handler(GtkWidget *widget)
{
    int j;

	if (widget == NULL) return FALSE;
	process_stat(nbcpu);

 	gtk_widget_queue_draw(widget);
  
  	return TRUE;
}


int main(int argc, char* argv[])
{
	int j;
	GtkWidget *window;          /* gtk windows */
    GtkWidget *darea;           /* cairo drawing area */
 


	gtk_init (&argc, &argv);    /* required with every gtk app */

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);  /* create window */
    darea = gtk_drawing_area_new();                 /* create cairo area */
    gtk_container_add (GTK_CONTAINER(window), darea);   /* add to window */

    /* connect callbacks to draw rectangle and close window/quit app */
    g_signal_connect (G_OBJECT(darea), "draw",
                        G_CALLBACK(on_draw_event), NULL);
    g_signal_connect (G_OBJECT(window), "destroy",
                        G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all (window);   /* show all windows */

	nbcpu = min(get_nprocs(), MAX_CPU);

	printf("This system has %d processors configured and "
    "%d processors available.\n", get_nprocs_conf(), nbcpu);
	
	str = (char *)malloc((size_t)STR_LEN);
	
	g_timeout_add(200, (GSourceFunc) time_handler, (gpointer) window);
	gtk_main();     /* pass control to gtk event-loop */

	free(str);
	return 0;
}
