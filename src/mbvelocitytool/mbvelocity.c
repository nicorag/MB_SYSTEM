
/* Begin user code block <abstract> */
/* End user code block <abstract> */

/**
 * README: Portions of this file are merged at file generation
 * time. Edits can be made *only* in between specified code blocks, look
 * for keywords <Begin user code> and <End user code>.
 */
/*
 * Generated by the ICS Builder Xcessory (BX).
 *
 * BuilderXcessory Version 6.1.3
 * Code Generator Xcessory 6.1.3 (08/19/04) CGX Scripts 6.1 Motif 2.1 
 *
 */


/* Begin user code block <file_comments> */
/* End user code block <file_comments> */

/*
 * Motif required Headers
 */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/RepType.h>
#include <Xm/MwmUtil.h>

/**
 * Globally included information.
 */


/*
 * Headers for classes used in this program
 */

/**
 * Common constant and pixmap declarations.
 */
#include "mbvelocity_creation.h"

/**
 * Convenience functions from utilities file.
 */
extern void RegisterBxConverters(XtAppContext);
extern XtPointer BX_CONVERT(Widget, char *, char *, int, Boolean *);
extern XtPointer BX_DOUBLE(double);
extern XtPointer BX_SINGLE(float);
extern void BX_MENU_POST(Widget, XtPointer, XEvent *, Boolean *);
extern Pixmap XPM_PIXMAP(Widget, char **);
extern void BX_SET_BACKGROUND_COLOR(Widget, ArgList, Cardinal *, Pixel);

/**
 * Declarations for shell callbacks.
 */
extern void do_quit(Widget, XtPointer, XtPointer);

/* Begin user code block <globals> */

#include "../../include/mb_define.h"
#include "mbvelocity.h"

Widget Createwindow_mbvelocity(Widget parent);

/* End user code block <globals> */

/**
 * Change this line via the Output Application Names Dialog.
 */
#define BX_APP_CLASS "MB-system"

int main( int argc, char **argv)
{
    Widget       parent;
    XtAppContext app;
    Arg          args[256];
    Cardinal     ac;
    Widget   topLevelShell;
    Widget   window_mbvelocity;
    
    /* Begin user code block <declarations> */
    
    /* make sure that the argc that goes to XtVaAppInitialize
       is 1 so that no options are removed by its option parsing */
    int		 argc_save;
    argc_save = argc;
    argc = 1;
    
    /* End user code block <declarations> */
    
    /*
     * Initialize Xt. 
     */
    
    XtSetLanguageProc(NULL, (XtLanguageProc) NULL, NULL); 
    
    /*
     * The applicationShell is created as an unrealized
     * parent for multiple topLevelShells.  The topLevelShells
     * are created as popup children of the applicationShell.
     * This is a recommendation of Paul Asente & Ralph Swick in
     * _X_Window_System_Toolkit_ p. 677.
     */
    
    parent = XtVaOpenApplication ( &app, 
                                   BX_APP_CLASS, 
                                   NULL, 
                                   0, 
                                   &argc, 
                                   argv, 
                                   NULL, 
                                   sessionShellWidgetClass, 
                                   NULL );
    
    RegisterBxConverters(app);
    XmRepTypeInstallTearOffModelConverter();
    
    /* Begin user code block <create_shells> */
    /* End user code block <create_shells> */
    
    /*
     * Create classes and widgets used in this program. 
     */
    
    /* Begin user code block <create_topLevelShell> */
    /* End user code block <create_topLevelShell> */
    
    ac = 0;
    XtSetArg(args[ac], XmNtitle, "MBvelocityTool"); ac++;
    XtSetArg(args[ac], XmNiconName, "MBvelocitytool"); ac++;
    XtSetArg(args[ac], XmNx, 365); ac++;
    XtSetArg(args[ac], XmNy, 192); ac++;
    XtSetArg(args[ac], XmNwidth, 1016); ac++;
    XtSetArg(args[ac], XmNheight, 668); ac++;
    topLevelShell = XtCreatePopupShell((char *)"topLevelShell",
        topLevelShellWidgetClass,
        parent,
        args, 
        ac);
    XtAddCallback(topLevelShell, XmNdestroyCallback, do_quit, (XtPointer)0);
    window_mbvelocity = (Widget)Createwindow_mbvelocity(topLevelShell);
    XtManageChild(window_mbvelocity);
    XtPopup(XtParent(window_mbvelocity), XtGrabNone);
    
    /* Begin user code block <app_procedures> */
    
    /* initialize app value and wait until view realized */
    do_wait_until_viewed(app);
    
    /* initialize everything */
    do_mbvelocity_init(argc_save,argv);
    
    /* End user code block <app_procedures> */
    
    /* Begin user code block <main_loop> */
    /* End user code block <main_loop> */
    
    XtAppMainLoop(app);
    
    /*
     * A return value regardless of whether or not the main loop ends. 
     */
     return(0); 
}
