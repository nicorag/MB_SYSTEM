/*--------------------------------------------------------------------
 *    The MB-system:	mbedit_stubs.c	3.00	4/8/93
 *    $Id: mbedit_stubs.c,v 3.0 1993-05-14 23:36:46 sohara Exp $
 *
 *    Copyright (c) 1993 by 
 *    D. W. Caress (caress@lamont.ldgo.columbia.edu)
 *    and D. N. Chayes (dale@lamont.ldgo.columbia.edu)
 *    Lamont-Doherty Earth Observatory
 *    Palisades, NY  10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * MBEDIT is an interactive beam editor for multibeam bathymetry data.
 * It can work with any data format supported by the MBIO library.
 * This version uses the XVIEW toolkit and has been developed using
 * the DEVGUIDE package.  A future version will employ the MOTIF
 * toolkit for greater portability.  This file contains the user
 * interface related code - the companion file mbedit.c contains
 * the code that does not directly depend on the XVIEW interface.
 *
 * Author:	D. W. Caress
 * Date:	April 8, 1993
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  1993/05/14  23:30:52  sohara
 * Initial revision
 *
 */


/*--------------------------------------------------------------------*/
/*
 * mbedit_stubs.c - Notify and event callback function stubs.
 * This file was generated by `gxv' from `mbedit.G'.
 */

/* standard include files */
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/svrimage.h>
#include <xview/cms.h>
#include <sys/dir.h>

/* XVIEW UI include file for this application */
#include "mbedit_ui.h"

/*
 * Global object definitions.
 */
mbedit_window_mbedit_objects	*Mbedit_window_mbedit;
mbedit_popup_load_objects	*Mbedit_popup_load;

/* Global Xwindows graphics parameters */
Display	*dpy;			/* Xwindows display/screen number */
Window	can_xid;		/* Xwindows window id for mbedit canvas */
Window	frm_xid;		/* Xwindows window id for mbedit frame */
int	can_xgid;		/* XG graphics id */
int	borders[4];		/* Canvas borders */
int	ncolors;
int	colors[256][3];
char	*fontname = "8x13";
Server_image	svr_image;
Icon	icon;
Xv_singlecolor	black, white;
static Xv_Cursor	cursor;
static short 	crosshair[] = {
#include "cursor.h"
};
static short icon_image[] = {
#include "mbedit.icon"
};

/* Global mbedit definitions */
int	plot_size_max;
int	plot_size;
int	buffer_size_max;
int	buffer_size;
int	hold_size;
int	format;
int	step;
int	nloaded;
int	ndumped;
int	nbuffer;
int	ngood;
int	icurrent;
int	nplot;
int	scale_max;
int	xscale;
int	yscale;
int	x_interval;
int	y_interval;
int	status;

/* file opening parameters */
int	startup_file = 0;
int	open_files_count = 0;
struct direct **open_files;

#ifdef MAIN

/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute	INSTANCE;

main(argc, argv)
	int	argc;
	char	**argv;
{
  static char rcs_id[]="$Id: mbedit_stubs.c,v 3.0 1993-05-14 23:36:46 sohara Exp $";
	int	status;
	int	i;

	/*
	 * Initialize XView.
	 */
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);
	INSTANCE = xv_unique_key();
	
	/*
	 * Initialize user interface components.
	 * Do NOT edit the object initializations by hand.
	 */
	Mbedit_window_mbedit = mbedit_window_mbedit_objects_initialize(NULL, NULL);
	Mbedit_popup_load = mbedit_popup_load_objects_initialize(NULL, Mbedit_window_mbedit->window_mbedit);

	/* set up graphics initialization */
	dpy      = (Display *)xv_get(Mbedit_window_mbedit->canvas_mbedit, XV_DISPLAY);
	frm_xid  = (Window)xv_get(Mbedit_window_mbedit->window_mbedit,XV_XID);
	can_xid  = (Window)xv_get(canvas_paint_window(
			Mbedit_window_mbedit->canvas_mbedit),XV_XID);
	borders[0] = 0;
	borders[1] = (int) xv_get(Mbedit_window_mbedit->canvas_mbedit,CANVAS_WIDTH)-1;
	borders[2] = 0;
	borders[3] = (int) xv_get(Mbedit_window_mbedit->canvas_mbedit,CANVAS_HEIGHT)-1;

	/* set cursor */
	svr_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
		XV_WIDTH,           64,
		XV_HEIGHT,          64,
		SERVER_IMAGE_BITS,  crosshair,
		NULL); 
	white.red = white.green = white.blue = 255;
	black.red = black.green = black.blue = 0;
	cursor = (Xv_Cursor)xv_create(XV_NULL, CURSOR,
		CURSOR_IMAGE,               svr_image,
		CURSOR_FOREGROUND_COLOR,    &black,
		CURSOR_BACKGROUND_COLOR,    &white,
		CURSOR_XHOT,                32,
		CURSOR_YHOT,                32,
		NULL);
	xv_set(canvas_paint_window(Mbedit_window_mbedit->canvas_mbedit),
		WIN_CURSOR,         cursor,
		NULL);

	/* set up program icon */
	svr_image = (Server_image)xv_create(NULL, SERVER_IMAGE,
		XV_WIDTH,		64,
		XV_HEIGHT,		64,
		SERVER_IMAGE_BITS,	icon_image,
		NULL);
	icon = (Icon)xv_create(Mbedit_window_mbedit->window_mbedit, ICON,
		ICON_IMAGE,		svr_image,
		NULL);
	xv_set(Mbedit_window_mbedit->window_mbedit, FRAME_ICON, icon, NULL);

	/* set up colormap */
	colors[0][0] = 255;
	colors[0][1] = 255;
	colors[0][2] = 255;
	colors[1][0] = 0;
	colors[1][1] = 0;
	colors[1][2] = 0;
	ncolors = 2;
	for (i=0;i<8;i++)
		{
		colors[i+ncolors][0] = 0;
		colors[i+ncolors][1] = (int) 255./7.*i;
		colors[i+ncolors][2] = 255;
		}
	ncolors = ncolors + 8;
	for (i=0;i<8;i++)
		{
		colors[i+ncolors][0] = 0;
		colors[i+ncolors][1] = 255;
		colors[i+ncolors][2] = 255 - (int) 255./8.*i;
		}
	ncolors = ncolors + 8;
	for (i=0;i<8;i++)
		{
		colors[i+ncolors][0] = (int) 255./8.*i;
		colors[i+ncolors][1] = 255;
		colors[i+ncolors][2] = 0;
		}
	ncolors = ncolors + 8;
	for (i=0;i<8;i++)
		{
		colors[i+ncolors][0] = 255;
		colors[i+ncolors][1] = 255 - (int) 255./8.*i;
		colors[i+ncolors][2] = 0;
		}
	ncolors = ncolors + 8;

	/* initialize graphics */
	can_xgid = xg_init(dpy, can_xid, borders, colors, ncolors, fontname);
	(void) xg_setwincolormap(can_xgid, frm_xid);
	status = mbedit_set_graphics(can_xgid, borders);

	/* initialize mbedit proper */
	status = mbedit_init(argc,argv,&startup_file);

	/* get some default values from mbedit */
	status = mbedit_get_defaults(&plot_size_max,&plot_size,
			&buffer_size_max,&buffer_size,
			&hold_size,&format,
			&scale_max,&xscale,&yscale,
			&x_interval,&y_interval);

	/* set appropriate graphical control values */
	xv_set(Mbedit_window_mbedit->slider_number_pings,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, plot_size_max,
		PANEL_VALUE, plot_size,
		NULL);
	xv_set(Mbedit_window_mbedit->slider_buffer_size,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, buffer_size_max,
		PANEL_VALUE, buffer_size,
		NULL);
	xv_set(Mbedit_window_mbedit->slider_buffer_hold,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, buffer_size_max/2,
		PANEL_VALUE, hold_size,
		NULL);
	step = plot_size/2;
	xv_set(Mbedit_window_mbedit->textfield_number_step,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, plot_size,
		PANEL_VALUE, step,
		NULL);
	xv_set(Mbedit_popup_load->textfield_format,
		PANEL_VALUE, format,
		NULL);
	xv_set(Mbedit_window_mbedit->slider_scale_x,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, scale_max,
		PANEL_VALUE, xscale,
		NULL);
	xv_set(Mbedit_window_mbedit->slider_scale_y,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, scale_max,
		PANEL_VALUE, yscale,
		NULL);
	xv_set(Mbedit_window_mbedit->textfield_x_interval,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, 10000,
		PANEL_VALUE, x_interval,
		NULL);
	xv_set(Mbedit_window_mbedit->textfield_y_interval,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, 10000,
		PANEL_VALUE, y_interval,
		NULL);

	/*
	 * Turn control over to XView.
	 */
	xv_main_loop(Mbedit_window_mbedit->window_mbedit);
	exit(0);
}

#endif


/*
 * File selector routine called by scandir().
 * Return TRUE if filename is not "." or "..".
 */
int
open_files_select(entry)
	struct direct	*entry;
{
	char		*ptr;

	if ((strcmp(entry->d_name, ".") == 0) ||
	    (strcmp(entry->d_name, "..") == 0))
		return (FALSE);
	else
		return(TRUE);
}

/*
 * Menu handler for `menu_file (Load)'.
 */
Menu_item
mbedit_menu_file_item0_callback(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	/* local variables */
	int	alphasort();
	int	i;

	mbedit_window_mbedit_objects * ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:

		/* delete old scrolling list */
		if (open_files_count > 0)
			{
			xv_set(Mbedit_popup_load->list_files,
				PANEL_LIST_DELETE_ROWS, 0, open_files_count,
				NULL);
			open_files_count = 0;
			}

		/* load filenames into scrolling list */
		open_files_count = scandir(".",&open_files,
			open_files_select, alphasort);
		for (i=0;i<open_files_count;i++)
			{
			xv_set(Mbedit_popup_load->list_files,
				PANEL_LIST_INSERT, i,
				PANEL_LIST_STRING, i, open_files[i]->d_name,
				NULL);
			}

		/* gxv_start_connections DO NOT EDIT THIS SECTION */

		xv_set(Mbedit_popup_load->popup_load, XV_SHOW, TRUE, NULL);
		
		/* gxv_end_connections */

		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

/*
 * Notify callback function for `slider_number_pings'.
 */
void
set_number_pings(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	plot_size = value;
	status = mbedit_action_plot(xscale,yscale,
		x_interval,y_interval,plot_size,&nbuffer,
		&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `button_done'.
 */
void
do_done(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	status = mbedit_action_done(buffer_size,&ndumped,&nloaded,
			&nbuffer,&ngood,&icurrent);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `slider_scale_x'.
 */
void
set_scale_x(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	xscale = value;
	status = mbedit_action_plot(xscale,yscale,
		x_interval,y_interval,plot_size,&nbuffer,
		&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `button_quit'.
 */
void
do_quit(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	status = mbedit_action_quit(buffer_size,&ndumped,&nloaded,
			&nbuffer,&ngood,&icurrent);
	if (status == 0) XBell(dpy,100);
	
	if (xv_destroy_safe(Mbedit_window_mbedit->window_mbedit) == XV_OK)
		exit(0);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `slider_scale_y'.
 */
void
set_scale_y(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	yscale = value;
	status = mbedit_action_plot(xscale,yscale,
		x_interval,y_interval,plot_size,&nbuffer,
		&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `textfield_number_step'.
 */
Panel_setting
set_number_step(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	int	value = (int) xv_get(item, PANEL_VALUE);
	
	step = (int) xv_get(Mbedit_window_mbedit->textfield_number_step,
		PANEL_VALUE);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	return panel_text_notify(item, event);
}

/*
 * Notify callback function for `button_load_cancel'.
 */
void
do_load_cancel(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_popup_load_objects *ip = (mbedit_popup_load_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
		
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	xv_set(Mbedit_popup_load->popup_load, XV_SHOW, FALSE, NULL);
	
	/* gxv_end_connections */

}

/*
 * Notify callback function for `button_load_ok'.
 */
void
do_load_ok(item, event)
	Panel_item	item;
	Event		*event;
{
	/* local definitions */
	int	selected;

	mbedit_popup_load_objects *ip = (mbedit_popup_load_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	/* get selected filename id and file format id */
	selected = (int) xv_get(Mbedit_popup_load->list_files,
				PANEL_LIST_FIRST_SELECTED);
	format = (int) xv_get(Mbedit_popup_load->textfield_format,
				PANEL_VALUE);

	/* get current number of pings to be plotted */
	plot_size = (int) xv_get(Mbedit_window_mbedit->slider_number_pings,
				PANEL_VALUE);

	/* deal with it */
	if (selected > -1)
		{
		status = mbedit_action_open(open_files[selected]->d_name,
				format,buffer_size,
				xscale,yscale,x_interval,y_interval,plot_size,
				&nloaded,&nbuffer,
				&ngood,&icurrent,&nplot);
		if (status == 0) XBell(dpy,100);
		}
	else
		fprintf(stderr,"\nno input multibeam file selected\n");
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	xv_set(Mbedit_popup_load->popup_load, XV_SHOW, FALSE, NULL);
	
	/* gxv_end_connections */

}

/*
 * Notify callback function for `button_next_buffer'.
 */
void
do_next_buffer(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	status = mbedit_action_next_buffer(hold_size,buffer_size,
			xscale,yscale,x_interval,y_interval,plot_size,
			&ndumped,&nloaded,&nbuffer,
			&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `button_forward'.
 */
void
do_forward(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	status = mbedit_action_step(step,xscale,yscale,x_interval,y_interval,plot_size,&nbuffer,
		&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `button_reverse'.
 */
void
do_reverse(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	status = mbedit_action_step(-step,xscale,yscale,x_interval,y_interval,plot_size,&nbuffer,
		&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `slider_buffer_size'.
 */
void
do_buffer_size(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	buffer_size = value;
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `slider_buffer_hold'.
 */
void
do_buffer_hold(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	hold_size = value;
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `textfield_x_interval'.
 */
Panel_setting
do_x_interval(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	int	value = (int) xv_get(item, PANEL_VALUE);
	
	x_interval = value;
	status = mbedit_action_plot(xscale,yscale,
		x_interval,y_interval,plot_size,&nbuffer,
		&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	return panel_text_notify(item, event);
}

/*
 * Notify callback function for `textfield_y_interval'.
 */
Panel_setting
do_y_interval(item, event)
	Panel_item	item;
	Event		*event;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	int	value = (int) xv_get(item, PANEL_VALUE);
	
	y_interval = value;
	status = mbedit_action_plot(xscale,yscale,
		x_interval,y_interval,plot_size,&nbuffer,
		&ngood,&icurrent,&nplot);
	if (status == 0) XBell(dpy,100);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	return panel_text_notify(item, event);
}

/*
 * Event callback function for `canvas_mbedit'.
 */
Notify_value
do_event(win, event, arg, type)
	Xv_window	win;
	Event		*event;
	Notify_arg	arg;
	Notify_event_type type;
{
	mbedit_window_mbedit_objects *ip = (mbedit_window_mbedit_objects *) xv_get(xv_get(win, CANVAS_PAINT_CANVAS_WINDOW), XV_KEY_DATA, INSTANCE);

	/* check for data file loaded at startup */
	if (startup_file)
		{
		startup_file = 0;
		status = mbedit_action_plot(xscale,yscale,
			x_interval,y_interval,plot_size,&nbuffer,
			&ngood,&icurrent,&nplot);
		if (status == 0) XBell(dpy,100);
		}

	/* process events */
	switch (event_action(event))
		{
		case 'Z':
		case 'z':
		case 'B':
		case 'b':
			if (event_is_down(event))
			status = mbedit_action_bad_ping(
				xscale,yscale,
				x_interval,y_interval,plot_size,
				&nbuffer,&ngood,&icurrent,&nplot);
			break;
		case 'G':
		case 'g':
			if (event_is_down(event))
			status = mbedit_action_good_ping(
				xscale,yscale,
				x_interval,y_interval,plot_size,
				&nbuffer,&ngood,&icurrent,&nplot);
			break;
		case 'L':
		case 'l':
			if (event_is_down(event))
			status = mbedit_action_left_ping(
				xscale,yscale,
				x_interval,y_interval,plot_size,
				&nbuffer,&ngood,&icurrent,&nplot);
			break;
		case 'R':
		case 'r':
			if (event_is_down(event))
			status = mbedit_action_right_ping(
				xscale,yscale,
				x_interval,y_interval,plot_size,
				&nbuffer,&ngood,&icurrent,&nplot);
			break;
		case KBD_USE:
			break;
		case KBD_DONE:
			break;
		case LOC_MOVE:
			break;
		case LOC_DRAG:
			break;
		case LOC_WINENTER:
			break;
		case LOC_WINEXIT:
			break;
		case ACTION_SELECT:
		case MS_LEFT:
			if (event_is_down(event))
			status = mbedit_action_mouse(
				event_x(event),event_y(event),
				xscale,yscale,
				x_interval,y_interval,plot_size,
				&nbuffer,&ngood,&icurrent,&nplot);
			if (status == 0) XBell(dpy,100);
			break;
		case ACTION_ADJUST:
		case MS_MIDDLE:
			if (event_is_down(event))
			status = mbedit_action_step(-step,xscale,yscale,
					x_interval,y_interval,
					plot_size,&nbuffer,
					&ngood,&icurrent,&nplot);
			if (status == 0) XBell(dpy,100);
			break;
		case ACTION_MENU:
		case MS_RIGHT:
			if (event_is_down(event))
			status = mbedit_action_step(step,xscale,yscale,
					x_interval,y_interval,
					plot_size,&nbuffer,
					&ngood,&icurrent,&nplot);
			if (status == 0) XBell(dpy,100);
			break;
		default:
			break;
		}
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	return notify_next_event_func(win, (Notify_event) event, arg, type);
}
