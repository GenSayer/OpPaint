/*
 * paint_x11.c - X11/Motif Paint Program
 * Compatible with Motif 1.2.x, GCC 2.95.x, C89
 * 
 * Enhanced version with improved GUI matching Windows version
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/keysym.h>

#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/DrawnB.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/CascadeB.h>
#include <Xm/MessageB.h>
#include <Xm/FileSB.h>
#include <Xm/Frame.h>
#include <Xm/BulletinB.h>

#include "paint.h"

/* Brush sizes */
static const int g_brush_sizes[5] = { 1, 3, 7, 15, 25 };

/* Application resources */
typedef struct {
    XtAppContext app_context;
    Display* display;
    Window root;
    int screen;
    int depth;
    Colormap colormap;
    Visual* visual;
    
    /* Widgets */
    Widget toplevel;
    Widget main_window;
    Widget menu_bar;
    Widget work_area;
    Widget tool_frame;
    Widget tool_rc;
    Widget size_rc;
    Widget canvas_frame;
    Widget canvas;
    Widget color_frame;
    Widget color_rc;
    Widget fgbg_da;
    Widget status_label;
    
    Widget tool_buttons[MAX_TOOLS];
    Widget color_buttons[MAX_COLORS];
    Widget size_buttons[5];
    
    /* Graphics */
    XImage* ximage;
    Pixmap buffer;
    GC gc;
    GC xor_gc;
    GC ui_gc;
    XFontStruct* ui_font;
    
    /* Colors */
    unsigned long pixels[MAX_COLORS];
    unsigned long white_pixel;
    unsigned long black_pixel;
    unsigned long gray_pixel;
    unsigned long highlight_pixel;
    unsigned long shadow_pixel;
    unsigned long select_pixel;
    
    /* State */
    PixelBuffer* pb_canvas;
    PixelBuffer* pb_temp;
    UndoStack* undo;
    PaintState state;
    int selected_size_index;
    
    /* 8-bit color cache: 6x6x6 color cube for fast lookup */
    unsigned long color_cube[216];
    int color_cube_valid;
    
    /* File dialog */
    Widget file_dialog;
    int file_dialog_mode;  /* 0=open, 1=save */
} AppData;

static AppData app;

/* Function prototypes - Initialization */
static void create_menu_bar(Widget parent);
static void create_tool_panel(Widget parent);
static void create_canvas(Widget parent);
static void create_color_panel(Widget parent);
static void create_status_bar(Widget parent);

static void init_colors(void);
static void init_canvas(void);
static void cleanup_canvas(void);
static void sync_ximage_from_pixbuf(void);
static void refresh_canvas(void);

/* Function prototypes - State updates */
static void update_title(void);
static void update_status(void);
static void update_tool_buttons(void);
static void update_size_buttons(void);
static void update_fgbg_display(void);
static void redraw_all_color_buttons(void);

/* Function prototypes - File operations */
static void do_file_new(void);
static void do_file_open(void);
static void do_file_save(void);
static void do_file_save_as(void);
static void do_undo(void);
static void do_clear(void);

/* Function prototypes - Drawing */
static void on_mouse_down(int x, int y, int button);
static void on_mouse_move(int x, int y, unsigned int button_state);
static void on_mouse_up(int x, int y, int button);
static void draw_current_tool(int x1, int y1, int x2, int y2, int to_temp);

/* Callbacks - Menu */
static void file_new_cb(Widget w, XtPointer client, XtPointer call);
static void file_open_cb(Widget w, XtPointer client, XtPointer call);
static void file_save_cb(Widget w, XtPointer client, XtPointer call);
static void file_save_as_cb(Widget w, XtPointer client, XtPointer call);
static void file_exit_cb(Widget w, XtPointer client, XtPointer call);
static void edit_undo_cb(Widget w, XtPointer client, XtPointer call);
static void edit_clear_cb(Widget w, XtPointer client, XtPointer call);
static void help_about_cb(Widget w, XtPointer client, XtPointer call);

/* Callbacks - Tools and colors */
static void tool_cb(Widget w, XtPointer client, XtPointer call);
static void size_cb(Widget w, XtPointer client, XtPointer call);

/* Callbacks - Canvas */
static void canvas_expose_cb(Widget w, XtPointer client, XtPointer call);
static void canvas_input_cb(Widget w, XtPointer client, XtPointer call);
static void canvas_motion_eh(Widget w, XtPointer client, XEvent* event, Boolean* cont);

/* Callbacks - Color buttons */
static void color_expose_cb(Widget w, XtPointer client, XtPointer call);
static void color_button_eh(Widget w, XtPointer client, XEvent* event, Boolean* cont);
static void fgbg_expose_cb(Widget w, XtPointer client, XtPointer call);

/* Callbacks - Tool button drawing */
static void tool_expose_cb(Widget w, XtPointer client, XtPointer call);
static void size_expose_cb(Widget w, XtPointer client, XtPointer call);

/* Callbacks - File dialog */
static void file_dialog_ok_cb(Widget w, XtPointer client, XtPointer call);
static void file_dialog_cancel_cb(Widget w, XtPointer client, XtPointer call);

/* ========================================================================== */
/* Main Function                                                              */
/* ========================================================================== */

int main(int argc, char** argv)
{
    Arg args[20];
    int n;
    
    /* Initialize application data */
    memset(&app, 0, sizeof(app));
    app.selected_size_index = 1;  /* Default to size 3 */
    
    /* Initialize Xt */
    n = 0;
    XtSetArg(args[n], XmNtitle, "Untitled - Paint"); n++;
    XtSetArg(args[n], XmNwidth, CANVAS_WIDTH + 130); n++;
    XtSetArg(args[n], XmNheight, CANVAS_HEIGHT + 170); n++;
    XtSetArg(args[n], XmNminWidth, 400); n++;
    XtSetArg(args[n], XmNminHeight, 300); n++;
    
    app.toplevel = XtAppInitialize(
        &app.app_context,
        "Paint",
        NULL, 0,
        &argc, argv,
        NULL,
        args, n);
    
    /* Get display info */
    app.display = XtDisplay(app.toplevel);
    app.screen = DefaultScreen(app.display);
    app.root = RootWindow(app.display, app.screen);
    app.depth = DefaultDepth(app.display, app.screen);
    app.colormap = DefaultColormap(app.display, app.screen);
    app.visual = DefaultVisual(app.display, app.screen);
    
    app.white_pixel = WhitePixel(app.display, app.screen);
    app.black_pixel = BlackPixel(app.display, app.screen);
    
    /* Initialize common data */
    paint_init_palette();
    paint_init_state(&app.state);
    app.state.brush_size = g_brush_sizes[app.selected_size_index];
    init_colors();
    
    /* Create main window */
    n = 0;
    app.main_window = XmCreateMainWindow(app.toplevel, "main", args, n);
    XtManageChild(app.main_window);
    
    /* Create menu bar */
    create_menu_bar(app.main_window);
    
    /* Create work area with form layout */
    n = 0;
    app.work_area = XmCreateForm(app.main_window, "work_area", args, n);
    XtManageChild(app.work_area);
    
    /* Create components - status bar first so others can attach to it */
    create_status_bar(app.work_area);
    create_color_panel(app.work_area);
    create_tool_panel(app.work_area);
    create_canvas(app.work_area);
    
    /* Set main window areas */
    XmMainWindowSetAreas(app.main_window, app.menu_bar, NULL, NULL, NULL, app.work_area);
    
    /* Realize toplevel */
    XtRealizeWidget(app.toplevel);
    
    /* Initialize canvas after realization */
    init_canvas();
    
    /* Enter main loop */
    XtAppMainLoop(app.app_context);
    
    return 0;
}

/* ========================================================================== */
/* Initialization Functions                                                   */
/* ========================================================================== */

static void init_colors(void)
{
    XColor color;
    int i;
    
    for (i = 0; i < MAX_COLORS; i++) {
        color.red = g_palette[i].r * 257;
        color.green = g_palette[i].g * 257;
        color.blue = g_palette[i].b * 257;
        color.flags = DoRed | DoGreen | DoBlue;
        
        if (XAllocColor(app.display, app.colormap, &color)) {
            app.pixels[i] = color.pixel;
        } else {
            /* Fallback */
            app.pixels[i] = (g_palette[i].r + g_palette[i].g + g_palette[i].b > 384) ?
                app.white_pixel : app.black_pixel;
        }
    }
    
    /* Allocate gray for UI elements */
    color.red = 49152;
    color.green = 49152;
    color.blue = 49152;
    if (XAllocColor(app.display, app.colormap, &color)) {
        app.gray_pixel = color.pixel;
    } else {
        app.gray_pixel = app.white_pixel;
    }
    
    /* Allocate highlight color (light) */
    color.red = 60000;
    color.green = 60000;
    color.blue = 60000;
    if (XAllocColor(app.display, app.colormap, &color)) {
        app.highlight_pixel = color.pixel;
    } else {
        app.highlight_pixel = app.white_pixel;
    }
    
    /* Allocate shadow color (dark) */
    color.red = 32768;
    color.green = 32768;
    color.blue = 32768;
    if (XAllocColor(app.display, app.colormap, &color)) {
        app.shadow_pixel = color.pixel;
    } else {
        app.shadow_pixel = app.black_pixel;
    }
    
    /* Allocate selection color (blue tint) */
    color.red = 45000;
    color.green = 50000;
    color.blue = 60000;
    if (XAllocColor(app.display, app.colormap, &color)) {
        app.select_pixel = color.pixel;
    } else {
        app.select_pixel = app.gray_pixel;
    }
}

static void init_canvas(void)
{
    XGCValues gcv;
    PaintColor white;
    Window win;
    
    /* Create pixel buffers */
    app.pb_canvas = pixbuf_create(CANVAS_WIDTH, CANVAS_HEIGHT);
    app.pb_temp = pixbuf_create(CANVAS_WIDTH, CANVAS_HEIGHT);
    
    /* Create undo stack */
    app.undo = undo_create();
    
    /* Clear to white */
    white.r = 255;
    white.g = 255;
    white.b = 255;
    pixbuf_clear(app.pb_canvas, white);
    pixbuf_clear(app.pb_temp, white);
    
    /* Get canvas window */
    win = XtWindow(app.canvas);
    
    /* Create XImage for display */
    app.ximage = XCreateImage(
        app.display,
        app.visual,
        app.depth,
        ZPixmap,
        0,
        NULL,
        CANVAS_WIDTH,
        CANVAS_HEIGHT,
        32,
        0);
    
    if (app.ximage) {
        app.ximage->data = (char*)malloc(app.ximage->bytes_per_line * CANVAS_HEIGHT);
        if (!app.ximage->data) {
            XDestroyImage(app.ximage);
            app.ximage = NULL;
        }
    }
    
    /* Create offscreen buffer */
    app.buffer = XCreatePixmap(
        app.display,
        win,
        CANVAS_WIDTH,
        CANVAS_HEIGHT,
        app.depth);
    
    /* Create main GC */
    gcv.foreground = app.pixels[0];
    gcv.background = app.pixels[14];
    gcv.line_width = 1;
    gcv.cap_style = CapRound;
    gcv.join_style = JoinRound;
    app.gc = XCreateGC(app.display, win,
                       GCForeground | GCBackground | GCLineWidth |
                       GCCapStyle | GCJoinStyle, &gcv);
    
    /* Create XOR GC for rubber banding */
    gcv.function = GXxor;
    gcv.foreground = app.pixels[0] ^ app.pixels[14];
    app.xor_gc = XCreateGC(app.display, win,
                           GCFunction | GCForeground | GCLineWidth |
                           GCCapStyle | GCJoinStyle, &gcv);
    
    /* Create UI GC for button drawing - reuse to avoid per-expose allocation */
    gcv.function = GXcopy;
    gcv.foreground = app.black_pixel;
    gcv.background = app.gray_pixel;
    gcv.line_width = 1;
    app.ui_gc = XCreateGC(app.display, app.root,
                          GCFunction | GCForeground | GCBackground | GCLineWidth,
                          &gcv);
    
    /* Load UI font once */
    app.ui_font = XLoadQueryFont(app.display, "fixed");
    if (!app.ui_font) {
        app.ui_font = XLoadQueryFont(app.display, "*");
    }
    if (app.ui_font) {
        XSetFont(app.display, app.ui_gc, app.ui_font->fid);
    }
    
    sync_ximage_from_pixbuf();
    refresh_canvas();
}

static void cleanup_canvas(void)
{
    if (app.ximage) {
        if (app.ximage->data) {
            free(app.ximage->data);
            app.ximage->data = NULL;
        }
        XDestroyImage(app.ximage);
        app.ximage = NULL;
    }
    
    if (app.buffer) {
        XFreePixmap(app.display, app.buffer);
        app.buffer = 0;
    }
    
    if (app.gc) {
        XFreeGC(app.display, app.gc);
        app.gc = NULL;
    }
    
    if (app.xor_gc) {
        XFreeGC(app.display, app.xor_gc);
        app.xor_gc = NULL;
    }
    
    if (app.ui_gc) {
        XFreeGC(app.display, app.ui_gc);
        app.ui_gc = NULL;
    }
    
    if (app.ui_font) {
        XFreeFont(app.display, app.ui_font);
        app.ui_font = NULL;
    }
    
    if (app.pb_canvas) {
        pixbuf_destroy(app.pb_canvas);
        app.pb_canvas = NULL;
    }
    
    if (app.pb_temp) {
        pixbuf_destroy(app.pb_temp);
        app.pb_temp = NULL;
    }
    
    if (app.undo) {
        undo_destroy(app.undo);
        app.undo = NULL;
    }
}

static void sync_ximage_from_pixbuf(void)
{
    int x, y;
    unsigned char* src_row;
    unsigned long pixel;
    int bpp;
    
    if (!app.pb_canvas || !app.ximage || !app.ximage->data) return;
    
    bpp = app.ximage->bits_per_pixel;
    
    /* Build 6x6x6 color cube for 8-bit displays (once) */
    if (bpp <= 8 && !app.color_cube_valid) {
        int ri, gi, bi;
        XColor xc;
        for (ri = 0; ri < 6; ri++) {
            for (gi = 0; gi < 6; gi++) {
                for (bi = 0; bi < 6; bi++) {
                    xc.red = ri * 13107;
                    xc.green = gi * 13107;
                    xc.blue = bi * 13107;
                    xc.flags = DoRed | DoGreen | DoBlue;
                    XAllocColor(app.display, app.colormap, &xc);
                    app.color_cube[ri * 36 + gi * 6 + bi] = xc.pixel;
                }
            }
        }
        app.color_cube_valid = 1;
    }
    
    for (y = 0; y < app.pb_canvas->height; y++) {
        src_row = app.pb_canvas->data + y * app.pb_canvas->row_stride;
        
        for (x = 0; x < app.pb_canvas->width; x++) {
            unsigned char r = src_row[x * 3 + 0];
            unsigned char g = src_row[x * 3 + 1];
            unsigned char b = src_row[x * 3 + 2];
            
            /* Create pixel value based on visual depth */
            if (bpp == 32 || bpp == 24) {
                pixel = ((unsigned long)r << 16) | ((unsigned long)g << 8) | (unsigned long)b;
            } else if (bpp == 16) {
                /* Assume 5-6-5 format */
                pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
            } else if (bpp == 15) {
                /* 5-5-5 format */
                pixel = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
            } else {
                /* 8-bit - use cached color cube lookup */
                int ri = (r * 5 + 127) / 255;
                int gi = (g * 5 + 127) / 255;
                int bi = (b * 5 + 127) / 255;
                pixel = app.color_cube[ri * 36 + gi * 6 + bi];
            }
            
            XPutPixel(app.ximage, x, y, pixel);
        }
    }
    
    /* Copy to buffer pixmap */
    if (app.buffer && app.gc) {
        XPutImage(app.display, app.buffer, app.gc,
                  app.ximage, 0, 0, 0, 0,
                  CANVAS_WIDTH, CANVAS_HEIGHT);
    }
}

static void refresh_canvas(void)
{
    if (XtIsRealized(app.canvas) && app.buffer) {
        XCopyArea(app.display, app.buffer, XtWindow(app.canvas), app.gc,
                  0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, 0, 0);
        XFlush(app.display);
    }
}

/* ========================================================================== */
/* Menu Bar Creation                                                          */
/* ========================================================================== */

static void create_menu_bar(Widget parent)
{
    Widget menu, cascade, button, sep;
    Arg args[10];
    int n;
    XmString str, accel;
    
    /* Create menu bar */
    n = 0;
    app.menu_bar = XmCreateMenuBar(parent, "menubar", args, n);
    XtManageChild(app.menu_bar);
    
    /* ===== File Menu ===== */
    n = 0;
    menu = XmCreatePulldownMenu(app.menu_bar, "file_menu", args, n);
    
    n = 0;
    str = XmStringCreateSimple("File");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNmnemonic, 'F'); n++;
    XtSetArg(args[n], XmNsubMenuId, menu); n++;
    cascade = XmCreateCascadeButton(app.menu_bar, "file", args, n);
    XtManageChild(cascade);
    XmStringFree(str);
    
    /* New */
    n = 0;
    str = XmStringCreateSimple("New");
    accel = XmStringCreateSimple("Ctrl+N");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNacceleratorText, accel); n++;
    XtSetArg(args[n], XmNaccelerator, "Ctrl<Key>n"); n++;
    XtSetArg(args[n], XmNmnemonic, 'N'); n++;
    button = XmCreatePushButton(menu, "new", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, file_new_cb, NULL);
    XmStringFree(str);
    XmStringFree(accel);
    
    /* Open */
    n = 0;
    str = XmStringCreateSimple("Open...");
    accel = XmStringCreateSimple("Ctrl+O");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNacceleratorText, accel); n++;
    XtSetArg(args[n], XmNaccelerator, "Ctrl<Key>o"); n++;
    XtSetArg(args[n], XmNmnemonic, 'O'); n++;
    button = XmCreatePushButton(menu, "open", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, file_open_cb, NULL);
    XmStringFree(str);
    XmStringFree(accel);
    
    /* Save */
    n = 0;
    str = XmStringCreateSimple("Save");
    accel = XmStringCreateSimple("Ctrl+S");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNacceleratorText, accel); n++;
    XtSetArg(args[n], XmNaccelerator, "Ctrl<Key>s"); n++;
    XtSetArg(args[n], XmNmnemonic, 'S'); n++;
    button = XmCreatePushButton(menu, "save", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, file_save_cb, NULL);
    XmStringFree(str);
    XmStringFree(accel);
    
    /* Save As */
    n = 0;
    str = XmStringCreateSimple("Save As...");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNmnemonic, 'A'); n++;
    button = XmCreatePushButton(menu, "saveas", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, file_save_as_cb, NULL);
    XmStringFree(str);
    
    /* Separator */
    n = 0;
    sep = XmCreateSeparatorGadget(menu, "sep1", args, n);
    XtManageChild(sep);
    
    /* Exit */
    n = 0;
    str = XmStringCreateSimple("Exit");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNmnemonic, 'x'); n++;
    button = XmCreatePushButton(menu, "exit", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, file_exit_cb, NULL);
    XmStringFree(str);
    
    /* ===== Edit Menu ===== */
    n = 0;
    menu = XmCreatePulldownMenu(app.menu_bar, "edit_menu", args, n);
    
    n = 0;
    str = XmStringCreateSimple("Edit");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNmnemonic, 'E'); n++;
    XtSetArg(args[n], XmNsubMenuId, menu); n++;
    cascade = XmCreateCascadeButton(app.menu_bar, "edit", args, n);
    XtManageChild(cascade);
    XmStringFree(str);
    
    /* Undo */
    n = 0;
    str = XmStringCreateSimple("Undo");
    accel = XmStringCreateSimple("Ctrl+Z");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNacceleratorText, accel); n++;
    XtSetArg(args[n], XmNaccelerator, "Ctrl<Key>z"); n++;
    XtSetArg(args[n], XmNmnemonic, 'U'); n++;
    button = XmCreatePushButton(menu, "undo", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, edit_undo_cb, NULL);
    XmStringFree(str);
    XmStringFree(accel);
    
    /* Separator */
    n = 0;
    sep = XmCreateSeparatorGadget(menu, "sep2", args, n);
    XtManageChild(sep);
    
    /* Clear All */
    n = 0;
    str = XmStringCreateSimple("Clear All");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNmnemonic, 'C'); n++;
    button = XmCreatePushButton(menu, "clear", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, edit_clear_cb, NULL);
    XmStringFree(str);
    
    /* ===== Help Menu ===== */
    n = 0;
    menu = XmCreatePulldownMenu(app.menu_bar, "help_menu", args, n);
    
    n = 0;
    str = XmStringCreateSimple("Help");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNmnemonic, 'H'); n++;
    XtSetArg(args[n], XmNsubMenuId, menu); n++;
    cascade = XmCreateCascadeButton(app.menu_bar, "help", args, n);
    XtManageChild(cascade);
    XmStringFree(str);
    
    /* Set as help menu */
    n = 0;
    XtSetArg(args[n], XmNmenuHelpWidget, cascade); n++;
    XtSetValues(app.menu_bar, args, n);
    
    /* About */
    n = 0;
    str = XmStringCreateSimple("About...");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNmnemonic, 'A'); n++;
    button = XmCreatePushButton(menu, "about", args, n);
    XtManageChild(button);
    XtAddCallback(button, XmNactivateCallback, help_about_cb, NULL);
    XmStringFree(str);
}

/* ========================================================================== */
/* Tool Panel Creation                                                        */
/* ========================================================================== */

static void create_tool_panel(Widget parent)
{
    Widget label, sep;
    Arg args[20];
    int n, i;
    XmString str;
    
    /* Tool frame - left side, above color frame */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, app.color_frame); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    XtSetArg(args[n], XmNbottomOffset, 2); n++;
    XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
    XtSetArg(args[n], XmNshadowThickness, 2); n++;
    app.tool_frame = XmCreateFrame(parent, "tool_frame", args, n);
    XtManageChild(app.tool_frame);
    
    /* Row column for tools */
    n = 0;
    XtSetArg(args[n], XmNorientation, XmVERTICAL); n++;
    XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
    XtSetArg(args[n], XmNspacing, 2); n++;
    XtSetArg(args[n], XmNmarginWidth, 4); n++;
    XtSetArg(args[n], XmNmarginHeight, 4); n++;
    app.tool_rc = XmCreateRowColumn(app.tool_frame, "tool_rc", args, n);
    XtManageChild(app.tool_rc);
    
    /* Tools label */
    n = 0;
    str = XmStringCreateSimple("Tools:");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
    label = XmCreateLabelGadget(app.tool_rc, "tools_lbl", args, n);
    XtManageChild(label);
    XmStringFree(str);
    
    /* Tool buttons - using DrawnButton for custom appearance */
    for (i = 0; i < MAX_TOOLS; i++) {
        n = 0;
        XtSetArg(args[n], XmNwidth, 75); n++;
        XtSetArg(args[n], XmNheight, 22); n++;
        XtSetArg(args[n], XmNpushButtonEnabled, True); n++;
        XtSetArg(args[n], XmNshadowThickness, 2); n++;
        app.tool_buttons[i] = XmCreateDrawnButton(app.tool_rc, "tool", args, n);
        XtManageChild(app.tool_buttons[i]);
        XtAddCallback(app.tool_buttons[i], XmNactivateCallback,
                     tool_cb, (XtPointer)(long)i);
        XtAddCallback(app.tool_buttons[i], XmNexposeCallback,
                     tool_expose_cb, (XtPointer)(long)i);
    }
    
    /* Separator */
    n = 0;
    sep = XmCreateSeparatorGadget(app.tool_rc, "sep1", args, n);
    XtManageChild(sep);
    
    /* Brush Size label */
    n = 0;
    str = XmStringCreateSimple("Brush Size:");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
    label = XmCreateLabelGadget(app.tool_rc, "size_lbl", args, n);
    XtManageChild(label);
    XmStringFree(str);
    
    /* Size buttons row column */
    n = 0;
    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
    XtSetArg(args[n], XmNpacking, XmPACK_TIGHT); n++;
    XtSetArg(args[n], XmNspacing, 2); n++;
    app.size_rc = XmCreateRowColumn(app.tool_rc, "size_rc", args, n);
    XtManageChild(app.size_rc);
    
    /* Size buttons - using DrawnButton */
    for (i = 0; i < 5; i++) {
        n = 0;
        XtSetArg(args[n], XmNwidth, 22); n++;
        XtSetArg(args[n], XmNheight, 22); n++;
        XtSetArg(args[n], XmNpushButtonEnabled, True); n++;
        XtSetArg(args[n], XmNshadowThickness, 2); n++;
        app.size_buttons[i] = XmCreateDrawnButton(app.size_rc, "size", args, n);
        XtManageChild(app.size_buttons[i]);
        XtAddCallback(app.size_buttons[i], XmNactivateCallback,
                     size_cb, (XtPointer)(long)i);
        XtAddCallback(app.size_buttons[i], XmNexposeCallback,
                     size_expose_cb, (XtPointer)(long)i);
    }
}

/* ========================================================================== */
/* Canvas Creation                                                            */
/* ========================================================================== */

static void create_canvas(Widget parent)
{
    Arg args[20];
    int n;
    
    /* Canvas frame - right of tool frame, above color frame */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, app.tool_frame); n++;
    XtSetArg(args[n], XmNleftOffset, 4); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, app.color_frame); n++;
    XtSetArg(args[n], XmNbottomOffset, 2); n++;
    XtSetArg(args[n], XmNshadowType, XmSHADOW_IN); n++;
    XtSetArg(args[n], XmNshadowThickness, 2); n++;
    app.canvas_frame = XmCreateFrame(parent, "canvas_frame", args, n);
    XtManageChild(app.canvas_frame);
    
    /* Canvas drawing area */
    n = 0;
    XtSetArg(args[n], XmNwidth, CANVAS_WIDTH); n++;
    XtSetArg(args[n], XmNheight, CANVAS_HEIGHT); n++;
    XtSetArg(args[n], XmNbackground, app.white_pixel); n++;
    XtSetArg(args[n], XmNresizePolicy, XmRESIZE_NONE); n++;
    app.canvas = XmCreateDrawingArea(app.canvas_frame, "canvas", args, n);
    XtManageChild(app.canvas);
    
    /* Add callbacks */
    XtAddCallback(app.canvas, XmNexposeCallback, canvas_expose_cb, NULL);
    XtAddCallback(app.canvas, XmNinputCallback, canvas_input_cb, NULL);
    
    /* Add motion event handler */
    XtAddEventHandler(app.canvas,
                     ButtonMotionMask | PointerMotionMask,
                     False,
                     (XtEventHandler)canvas_motion_eh, NULL);
}

/* ========================================================================== */
/* Color Panel Creation                                                       */
/* ========================================================================== */

static void create_color_panel(Widget parent)
{
    Widget form, label;
    Arg args[20];
    int n, i;
    XmString str;
    
    /* Color frame - bottom, above status bar */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNbottomWidget, app.status_label); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    XtSetArg(args[n], XmNbottomOffset, 2); n++;
    XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN); n++;
    XtSetArg(args[n], XmNshadowThickness, 2); n++;
    app.color_frame = XmCreateFrame(parent, "color_frame", args, n);
    XtManageChild(app.color_frame);
    
    /* Form for layout inside color frame */
    n = 0;
    form = XmCreateForm(app.color_frame, "color_form", args, n);
    XtManageChild(form);
    
    /* FG/BG display area */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 4); n++;
    XtSetArg(args[n], XmNtopOffset, 4); n++;
    XtSetArg(args[n], XmNbottomOffset, 4); n++;
    XtSetArg(args[n], XmNwidth, 50); n++;
    XtSetArg(args[n], XmNheight, 45); n++;
    app.fgbg_da = XmCreateDrawingArea(form, "fgbg", args, n);
    XtManageChild(app.fgbg_da);
    XtAddCallback(app.fgbg_da, XmNexposeCallback, fgbg_expose_cb, NULL);
    
    /* Colors label */
    n = 0;
    str = XmStringCreateSimple("Colors (L=FG, R=BG):");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, app.fgbg_da); n++;
    XtSetArg(args[n], XmNleftOffset, 10); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    label = XmCreateLabelGadget(form, "colors_lbl", args, n);
    XtManageChild(label);
    XmStringFree(str);
    
    /* Color palette row column */
    n = 0;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, app.fgbg_da); n++;
    XtSetArg(args[n], XmNleftOffset, 10); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, label); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
    XtSetArg(args[n], XmNpacking, XmPACK_COLUMN); n++;
    XtSetArg(args[n], XmNnumColumns, 2); n++;
    XtSetArg(args[n], XmNspacing, 1); n++;
    app.color_rc = XmCreateRowColumn(form, "color_rc", args, n);
    XtManageChild(app.color_rc);
    
    /* Color buttons using DrawnButton for custom drawing */
    for (i = 0; i < MAX_COLORS; i++) {
        n = 0;
        XtSetArg(args[n], XmNwidth, COLOR_BTN_SIZE + 2); n++;
        XtSetArg(args[n], XmNheight, COLOR_BTN_SIZE + 2); n++;
        XtSetArg(args[n], XmNpushButtonEnabled, True); n++;
        XtSetArg(args[n], XmNshadowThickness, 1); n++;
        app.color_buttons[i] = XmCreateDrawnButton(app.color_rc, "color", args, n);
        XtManageChild(app.color_buttons[i]);
        XtAddCallback(app.color_buttons[i], XmNexposeCallback,
                     color_expose_cb, (XtPointer)(long)i);
        XtAddCallback(app.color_buttons[i], XmNactivateCallback,
                     tool_cb, NULL);  /* Dummy - we use event handler */
        /* Add event handler for button press to detect left/right click */
        XtAddEventHandler(app.color_buttons[i],
                         ButtonPressMask | ButtonReleaseMask,
                         False,
                         (XtEventHandler)color_button_eh, (XtPointer)(long)i);
    }
}

/* ========================================================================== */
/* Status Bar Creation                                                        */
/* ========================================================================== */

static void create_status_bar(Widget parent)
{
    Arg args[15];
    int n;
    XmString str;
    
    /* Status bar label at bottom */
    n = 0;
    str = XmStringCreateSimple("Ready");
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNrightOffset, 2); n++;
    XtSetArg(args[n], XmNbottomOffset, 2); n++;
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
    XtSetArg(args[n], XmNrecomputeSize, False); n++;
    app.status_label = XmCreateLabelGadget(parent, "status", args, n);
    XtManageChild(app.status_label);
    XmStringFree(str);
}

/* ========================================================================== */
/* State Update Functions                                                     */
/* ========================================================================== */

static void update_title(void)
{
    char title[300];
    const char* filename;
    Arg args[2];
    
    if (app.state.filename[0]) {
        filename = strrchr(app.state.filename, '/');
        if (filename) {
            filename++;
        } else {
            filename = app.state.filename;
        }
    } else {
        filename = "Untitled";
    }
    
    sprintf(title, "%s%s - Paint",
            app.state.modified ? "*" : "",
            filename);
    
    XtSetArg(args[0], XmNtitle, title);
    XtSetValues(app.toplevel, args, 1);
}

static void update_status(void)
{
    char buf[256];
    XmString str;
    Arg args[2];
    
    sprintf(buf, "Tool: %s | Size: %d | Pos: (%d, %d) | FG: %d | BG: %d",
            g_tool_names[app.state.current_tool],
            app.state.brush_size,
            app.state.current_point.x,
            app.state.current_point.y,
            app.state.foreground_color,
            app.state.background_color);
    
    str = XmStringCreateSimple(buf);
    XtSetArg(args[0], XmNlabelString, str);
    XtSetValues(app.status_label, args, 1);
    XmStringFree(str);
}

static void update_tool_buttons(void)
{
    int i;
    
    for (i = 0; i < MAX_TOOLS; i++) {
        if (XtIsRealized(app.tool_buttons[i])) {
            XClearArea(app.display, XtWindow(app.tool_buttons[i]), 0, 0, 0, 0, True);
        }
    }
}

static void update_size_buttons(void)
{
    int i;
    
    for (i = 0; i < 5; i++) {
        if (XtIsRealized(app.size_buttons[i])) {
            XClearArea(app.display, XtWindow(app.size_buttons[i]), 0, 0, 0, 0, True);
        }
    }
}

static void update_fgbg_display(void)
{
    if (XtIsRealized(app.fgbg_da)) {
        XClearArea(app.display, XtWindow(app.fgbg_da), 0, 0, 0, 0, True);
    }
}

static void redraw_all_color_buttons(void)
{
    int i;
    
    for (i = 0; i < MAX_COLORS; i++) {
        if (XtIsRealized(app.color_buttons[i])) {
            XClearArea(app.display, XtWindow(app.color_buttons[i]), 0, 0, 0, 0, True);
        }
    }
    
    update_fgbg_display();
}

/* ========================================================================== */
/* File Operations                                                            */
/* ========================================================================== */

static void do_file_new(void)
{
    PaintColor white;
    
    white.r = 255;
    white.g = 255;
    white.b = 255;
    pixbuf_clear(app.pb_canvas, white);
    memcpy(app.pb_temp->data, app.pb_canvas->data,
           app.pb_canvas->row_stride * app.pb_canvas->height);
    
    undo_clear(app.undo);
    
    app.state.filename[0] = '\0';
    app.state.modified = 0;
    
    sync_ximage_from_pixbuf();
    refresh_canvas();
    update_title();
}

static void do_file_open(void)
{
    Arg args[10];
    int n;
    XmString title, pattern;
    
    if (!app.file_dialog) {
        n = 0;
        title = XmStringCreateSimple("Open File");
        pattern = XmStringCreateSimple("*.bmp");
        XtSetArg(args[n], XmNdialogTitle, title); n++;
        XtSetArg(args[n], XmNpattern, pattern); n++;
        XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
        XtSetArg(args[n], XmNautoUnmanage, True); n++;
        
        app.file_dialog = XmCreateFileSelectionDialog(app.toplevel, "file_dialog", args, n);
        
        XtAddCallback(app.file_dialog, XmNokCallback, file_dialog_ok_cb, NULL);
        XtAddCallback(app.file_dialog, XmNcancelCallback, file_dialog_cancel_cb, NULL);
        
        XtUnmanageChild(XmFileSelectionBoxGetChild(app.file_dialog, XmDIALOG_HELP_BUTTON));
        
        XmStringFree(title);
        XmStringFree(pattern);
    } else {
        XmString title_str = XmStringCreateSimple("Open File");
        XtVaSetValues(app.file_dialog, XmNdialogTitle, title_str, NULL);
        XmStringFree(title_str);
    }
    
    app.file_dialog_mode = 0;  /* Open mode */
    XtManageChild(app.file_dialog);
}

static void do_file_save(void)
{
    if (app.state.filename[0] == '\0') {
        do_file_save_as();
        return;
    }
    
    if (!bmp_save(app.state.filename, app.pb_canvas)) {
        Widget dialog;
        Arg args[5];
        int n;
        XmString str;
        
        n = 0;
        str = XmStringCreateSimple("Failed to save file.");
        XtSetArg(args[n], XmNmessageString, str); n++;
        XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
        dialog = XmCreateErrorDialog(app.toplevel, "error", args, n);
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
        XtManageChild(dialog);
        XmStringFree(str);
        return;
    }
    
    app.state.modified = 0;
    update_title();
}

static void do_file_save_as(void)
{
    Arg args[10];
    int n;
    XmString title, pattern;
    
    if (!app.file_dialog) {
        n = 0;
        title = XmStringCreateSimple("Save File");
        pattern = XmStringCreateSimple("*.bmp");
        XtSetArg(args[n], XmNdialogTitle, title); n++;
        XtSetArg(args[n], XmNpattern, pattern); n++;
        XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
        XtSetArg(args[n], XmNautoUnmanage, True); n++;
        
        app.file_dialog = XmCreateFileSelectionDialog(app.toplevel, "file_dialog", args, n);
        
        XtAddCallback(app.file_dialog, XmNokCallback, file_dialog_ok_cb, NULL);
        XtAddCallback(app.file_dialog, XmNcancelCallback, file_dialog_cancel_cb, NULL);
        
        XtUnmanageChild(XmFileSelectionBoxGetChild(app.file_dialog, XmDIALOG_HELP_BUTTON));
        
        XmStringFree(title);
        XmStringFree(pattern);
    } else {
        XmString title_str = XmStringCreateSimple("Save File");
        XtVaSetValues(app.file_dialog, XmNdialogTitle, title_str, NULL);
        XmStringFree(title_str);
    }
    
    app.file_dialog_mode = 1;  /* Save mode */
    XtManageChild(app.file_dialog);
}

static void do_undo(void)
{
    PixelBuffer* prev;
    
    if (!undo_can_undo(app.undo)) return;
    
    prev = undo_pop(app.undo);
    if (prev) {
        pixbuf_destroy(app.pb_canvas);
        app.pb_canvas = prev;
        
        memcpy(app.pb_temp->data, app.pb_canvas->data,
               app.pb_canvas->row_stride * app.pb_canvas->height);
        
        app.state.modified = 1;
        sync_ximage_from_pixbuf();
        refresh_canvas();
        update_title();
    }
}

static void do_clear(void)
{
    PaintColor bg;
    
    undo_push(app.undo, app.pb_canvas);
    
    bg = g_palette[app.state.background_color];
    pixbuf_clear(app.pb_canvas, bg);
    memcpy(app.pb_temp->data, app.pb_canvas->data,
           app.pb_canvas->row_stride * app.pb_canvas->height);
    
    app.state.modified = 1;
    sync_ximage_from_pixbuf();
    refresh_canvas();
    update_title();
}

/* ========================================================================== */
/* Drawing Operations                                                         */
/* ========================================================================== */

static void on_mouse_down(int x, int y, int button)
{
    app.state.is_drawing = 1;
    app.state.start_point.x = x;
    app.state.start_point.y = y;
    app.state.last_point.x = x;
    app.state.last_point.y = y;
    app.state.current_point.x = x;
    app.state.current_point.y = y;
    
    /* Save for undo */
    undo_push(app.undo, app.pb_canvas);
    
    /* Copy canvas to temp */
    memcpy(app.pb_temp->data, app.pb_canvas->data,
           app.pb_canvas->row_stride * app.pb_canvas->height);
    
    if (app.state.current_tool == TOOL_FILL) {
        PaintColor fc = g_palette[app.state.foreground_color];
        flood_fill_area(app.pb_canvas, x, y, fc);
        memcpy(app.pb_temp->data, app.pb_canvas->data,
               app.pb_canvas->row_stride * app.pb_canvas->height);
        app.state.is_drawing = 0;
        app.state.modified = 1;
        sync_ximage_from_pixbuf();
        refresh_canvas();
        update_title();
    }
    else if (app.state.current_tool == TOOL_PICKER) {
        PaintColor picked = pixbuf_get_pixel(app.pb_canvas, x, y);
        int i, best = 0;
        int best_dist = 0x7FFFFFFF;
        
        for (i = 0; i < MAX_COLORS; i++) {
            int dr = (int)picked.r - (int)g_palette[i].r;
            int dg = (int)picked.g - (int)g_palette[i].g;
            int db = (int)picked.b - (int)g_palette[i].b;
            int dist = dr*dr + dg*dg + db*db;
            if (dist < best_dist) {
                best_dist = dist;
                best = i;
            }
        }
        
        if (button == 1) {
            app.state.foreground_color = best;
        } else {
            app.state.background_color = best;
        }
        
        app.state.is_drawing = 0;
        redraw_all_color_buttons();
        update_status();
        
        /* Pop undo since we didn't draw */
        undo_pop(app.undo);
    }
    else if (app.state.current_tool == TOOL_PENCIL ||
             app.state.current_tool == TOOL_BRUSH ||
             app.state.current_tool == TOOL_AIRBRUSH ||
             app.state.current_tool == TOOL_ERASER) {
        draw_current_tool(x, y, x, y, 0);
        sync_ximage_from_pixbuf();
        refresh_canvas();
    }
    
    update_status();
}

static void on_mouse_move(int x, int y, unsigned int button_state)
{
    x = clamp_int(x, 0, app.pb_canvas->width - 1);
    y = clamp_int(y, 0, app.pb_canvas->height - 1);
    
    app.state.current_point.x = x;
    app.state.current_point.y = y;
    
    if (app.state.is_drawing && (button_state & Button1Mask)) {
        switch (app.state.current_tool) {
        case TOOL_PENCIL:
        case TOOL_BRUSH:
        case TOOL_ERASER:
            draw_current_tool(app.state.last_point.x, app.state.last_point.y, x, y, 0);
            sync_ximage_from_pixbuf();
            refresh_canvas();
            break;
            
        case TOOL_AIRBRUSH:
            draw_current_tool(x, y, x, y, 0);
            sync_ximage_from_pixbuf();
            refresh_canvas();
            break;
            
        case TOOL_LINE:
        case TOOL_RECT:
        case TOOL_FILLRECT:
        case TOOL_ELLIPSE:
        case TOOL_FILLELLIPSE:
        case TOOL_ROUNDRECT:
        case TOOL_FILLROUNDRECT:
            /* Rubber-band */
            memcpy(app.pb_temp->data, app.pb_canvas->data,
                   app.pb_canvas->row_stride * app.pb_canvas->height);
            draw_current_tool(app.state.start_point.x, app.state.start_point.y, x, y, 1);
            {
                PixelBuffer* swap = app.pb_canvas;
                app.pb_canvas = app.pb_temp;
                sync_ximage_from_pixbuf();
                app.pb_canvas = swap;
            }
            refresh_canvas();
            break;
        }
        
        app.state.last_point.x = x;
        app.state.last_point.y = y;
    }
    
    update_status();
}

static void on_mouse_up(int x, int y, int button)
{
    if (!app.state.is_drawing) return;
    
    x = clamp_int(x, 0, app.pb_canvas->width - 1);
    y = clamp_int(y, 0, app.pb_canvas->height - 1);
    
    switch (app.state.current_tool) {
    case TOOL_LINE:
    case TOOL_RECT:
    case TOOL_FILLRECT:
    case TOOL_ELLIPSE:
    case TOOL_FILLELLIPSE:
    case TOOL_ROUNDRECT:
    case TOOL_FILLROUNDRECT:
        draw_current_tool(app.state.start_point.x, app.state.start_point.y, x, y, 0);
        break;
    }
    
    memcpy(app.pb_temp->data, app.pb_canvas->data,
           app.pb_canvas->row_stride * app.pb_canvas->height);
    
    app.state.is_drawing = 0;
    app.state.modified = 1;
    
    sync_ximage_from_pixbuf();
    refresh_canvas();
    update_title();
}

static void draw_current_tool(int x1, int y1, int x2, int y2, int to_temp)
{
    PixelBuffer* target = to_temp ? app.pb_temp : app.pb_canvas;
    PaintColor fg = g_palette[app.state.foreground_color];
    PaintColor bg = g_palette[app.state.background_color];
    int size = app.state.brush_size;
    int radius;
    
    switch (app.state.current_tool) {
    case TOOL_PENCIL:
        draw_line(target, x1, y1, x2, y2, fg, 1);
        break;
        
    case TOOL_BRUSH:
        draw_line(target, x1, y1, x2, y2, fg, size);
        break;
        
    case TOOL_AIRBRUSH:
        draw_airbrush(target, x2, y2, fg, size * 2, app.state.airbrush_density);
        break;
        
    case TOOL_LINE:
        draw_line(target, x1, y1, x2, y2, fg, size);
        break;
        
    case TOOL_RECT:
        draw_rect(target, x1, y1, x2, y2, fg, size, 0);
        break;
        
    case TOOL_FILLRECT:
        draw_rect(target, x1, y1, x2, y2, fg, size, 1);
        break;
        
    case TOOL_ELLIPSE:
        draw_ellipse(target, x1, y1, x2, y2, fg, size, 0);
        break;
        
    case TOOL_FILLELLIPSE:
        draw_ellipse(target, x1, y1, x2, y2, fg, size, 1);
        break;
        
    case TOOL_ROUNDRECT:
        radius = min_int(abs_int(x2 - x1), abs_int(y2 - y1)) / 4;
        if (radius < 5) radius = 5;
        draw_round_rect(target, x1, y1, x2, y2, fg, size, 0, radius);
        break;
        
    case TOOL_FILLROUNDRECT:
        radius = min_int(abs_int(x2 - x1), abs_int(y2 - y1)) / 4;
        if (radius < 5) radius = 5;
        draw_round_rect(target, x1, y1, x2, y2, fg, size, 1, radius);
        break;
        
    case TOOL_ERASER:
        draw_line(target, x1, y1, x2, y2, bg, size * 2);
        break;
    }
}

/* ========================================================================== */
/* Menu Callbacks                                                             */
/* ========================================================================== */

static void file_new_cb(Widget w, XtPointer client, XtPointer call)
{
    do_file_new();
}

static void file_open_cb(Widget w, XtPointer client, XtPointer call)
{
    do_file_open();
}

static void file_save_cb(Widget w, XtPointer client, XtPointer call)
{
    do_file_save();
}

static void file_save_as_cb(Widget w, XtPointer client, XtPointer call)
{
    do_file_save_as();
}

static void file_exit_cb(Widget w, XtPointer client, XtPointer call)
{
    cleanup_canvas();
    exit(0);
}

static void edit_undo_cb(Widget w, XtPointer client, XtPointer call)
{
    do_undo();
}

static void edit_clear_cb(Widget w, XtPointer client, XtPointer call)
{
    do_clear();
}

static void help_about_cb(Widget w, XtPointer client, XtPointer call)
{
    Widget dialog;
    Arg args[5];
    int n;
    XmString str, title;
    
    str = XmStringCreateLtoR(
        "Portable Paint Program v2.0\n\n"
        "A full-featured MS Paint-like application\n"
        "Compatible with Win32 and X11/Motif\n\n"
        "Tools: Pencil, Brush, Airbrush, Line,\n"
        "Rectangle, Ellipse, Rounded Rect,\n"
        "Eraser, Fill, Color Picker\n\n"
        "Left-click: Draw with foreground color\n"
        "Right-click: Draw with background color\n"
        "Right-click on palette: Set background color",
        XmFONTLIST_DEFAULT_TAG);
    title = XmStringCreateSimple("About Paint");
    
    n = 0;
    XtSetArg(args[n], XmNmessageString, str); n++;
    XtSetArg(args[n], XmNdialogTitle, title); n++;
    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
    
    dialog = XmCreateInformationDialog(app.toplevel, "about", args, n);
    
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    
    XtManageChild(dialog);
    
    XmStringFree(str);
    XmStringFree(title);
}

/* ========================================================================== */
/* Tool and Size Callbacks                                                    */
/* ========================================================================== */

static void tool_cb(Widget w, XtPointer client, XtPointer call)
{
    int tool = (int)(long)client;
    
    app.state.current_tool = tool;
    update_tool_buttons();
    update_status();
}

static void size_cb(Widget w, XtPointer client, XtPointer call)
{
    int size_idx = (int)(long)client;
    
    app.selected_size_index = size_idx;
    app.state.brush_size = g_brush_sizes[size_idx];
    update_size_buttons();
    update_status();
}

/* ========================================================================== */
/* Tool Button Drawing Callback                                               */
/* ========================================================================== */

static void tool_expose_cb(Widget w, XtPointer client, XtPointer call)
{
    XmDrawnButtonCallbackStruct* cbs = (XmDrawnButtonCallbackStruct*)call;
    int tool_idx = (int)(long)client;
    Window win;
    Dimension width, height;
    Arg args[4];
    int is_selected;
    int text_x, text_y;
    int text_width;
    const char* label;
    
    static const char* tool_labels[MAX_TOOLS] = {
        "Pencil", "Brush", "Airbrush", "Line", "Rect", "FillRect",
        "Ellipse", "FillElps", "RndRect", "FillRnd", "Eraser", "Fill", "Picker"
    };
    
    if (!cbs || cbs->reason != XmCR_EXPOSE) return;
    if (!app.ui_gc) return;
    
    win = XtWindow(w);
    if (!win) return;
    
    XtSetArg(args[0], XmNwidth, &width);
    XtSetArg(args[1], XmNheight, &height);
    XtGetValues(w, args, 2);
    
    is_selected = (tool_idx == app.state.current_tool);
    label = tool_labels[tool_idx];
    
    /* Background */
    if (is_selected) {
        XSetForeground(app.display, app.ui_gc, app.select_pixel);
    } else {
        XSetForeground(app.display, app.ui_gc, app.gray_pixel);
    }
    XFillRectangle(app.display, win, app.ui_gc, 2, 2, width - 4, height - 4);
    
    /* 3D border effect */
    if (is_selected) {
        /* Sunken look */
        XSetForeground(app.display, app.ui_gc, app.shadow_pixel);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, width - 1, 0);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, 0, height - 1);
        XSetForeground(app.display, app.ui_gc, app.highlight_pixel);
        XDrawLine(app.display, win, app.ui_gc, width - 1, 0, width - 1, height - 1);
        XDrawLine(app.display, win, app.ui_gc, 0, height - 1, width - 1, height - 1);
    } else {
        /* Raised look */
        XSetForeground(app.display, app.ui_gc, app.highlight_pixel);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, width - 1, 0);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, 0, height - 1);
        XSetForeground(app.display, app.ui_gc, app.shadow_pixel);
        XDrawLine(app.display, win, app.ui_gc, width - 1, 0, width - 1, height - 1);
        XDrawLine(app.display, win, app.ui_gc, 0, height - 1, width - 1, height - 1);
    }
    
    /* Draw text label */
    if (app.ui_font) {
        text_width = XTextWidth(app.ui_font, label, strlen(label));
        text_x = (width - text_width) / 2;
        text_y = (height + app.ui_font->ascent - app.ui_font->descent) / 2;
        
        XSetForeground(app.display, app.ui_gc, app.black_pixel);
        XDrawString(app.display, win, app.ui_gc, text_x, text_y, label, strlen(label));
    }
}

/* ========================================================================== */
/* Size Button Drawing Callback                                               */
/* ========================================================================== */

static void size_expose_cb(Widget w, XtPointer client, XtPointer call)
{
    XmDrawnButtonCallbackStruct* cbs = (XmDrawnButtonCallbackStruct*)call;
    int size_idx = (int)(long)client;
    Window win;
    Dimension width, height;
    Arg args[4];
    int is_selected;
    char label[8];
    int text_x, text_y;
    int text_width;
    
    if (!cbs || cbs->reason != XmCR_EXPOSE) return;
    if (!app.ui_gc) return;
    
    win = XtWindow(w);
    if (!win) return;
    
    XtSetArg(args[0], XmNwidth, &width);
    XtSetArg(args[1], XmNheight, &height);
    XtGetValues(w, args, 2);
    
    is_selected = (size_idx == app.selected_size_index);
    sprintf(label, "%d", g_brush_sizes[size_idx]);
    
    /* Background */
    if (is_selected) {
        XSetForeground(app.display, app.ui_gc, app.select_pixel);
    } else {
        XSetForeground(app.display, app.ui_gc, app.gray_pixel);
    }
    XFillRectangle(app.display, win, app.ui_gc, 2, 2, width - 4, height - 4);
    
    /* 3D border */
    if (is_selected) {
        XSetForeground(app.display, app.ui_gc, app.shadow_pixel);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, width - 1, 0);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, 0, height - 1);
        XSetForeground(app.display, app.ui_gc, app.highlight_pixel);
        XDrawLine(app.display, win, app.ui_gc, width - 1, 0, width - 1, height - 1);
        XDrawLine(app.display, win, app.ui_gc, 0, height - 1, width - 1, height - 1);
    } else {
        XSetForeground(app.display, app.ui_gc, app.highlight_pixel);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, width - 1, 0);
        XDrawLine(app.display, win, app.ui_gc, 0, 0, 0, height - 1);
        XSetForeground(app.display, app.ui_gc, app.shadow_pixel);
        XDrawLine(app.display, win, app.ui_gc, width - 1, 0, width - 1, height - 1);
        XDrawLine(app.display, win, app.ui_gc, 0, height - 1, width - 1, height - 1);
    }
    
    /* Draw size label */
    if (app.ui_font) {
        text_width = XTextWidth(app.ui_font, label, strlen(label));
        text_x = (width - text_width) / 2;
        text_y = (height + app.ui_font->ascent - app.ui_font->descent) / 2;
        
        XSetForeground(app.display, app.ui_gc, app.black_pixel);
        XDrawString(app.display, win, app.ui_gc, text_x, text_y, label, strlen(label));
    }
}

/* ========================================================================== */
/* Color Button Event Handler and Expose Callback                             */
/* ========================================================================== */

static void color_button_eh(Widget w, XtPointer client, XEvent* event, Boolean* cont)
{
    int color_idx = (int)(long)client;
    
    if (event->type == ButtonPress) {
        if (event->xbutton.button == Button1) {
            /* Left click - set foreground color */
            app.state.foreground_color = color_idx;
            redraw_all_color_buttons();
            update_status();
        } else if (event->xbutton.button == Button3) {
            /* Right click - set background color */
            app.state.background_color = color_idx;
            redraw_all_color_buttons();
            update_status();
        }
    }
    
    *cont = True;
}

static void color_expose_cb(Widget w, XtPointer client, XtPointer call)
{
    XmDrawnButtonCallbackStruct* cbs = (XmDrawnButtonCallbackStruct*)call;
    int color_idx = (int)(long)client;
    Window win;
    Dimension width, height;
    Arg args[4];
    int is_fg, is_bg;
    
    if (!cbs || cbs->reason != XmCR_EXPOSE) return;
    if (!app.ui_gc) return;
    
    win = XtWindow(w);
    if (!win) return;
    
    XtSetArg(args[0], XmNwidth, &width);
    XtSetArg(args[1], XmNheight, &height);
    XtGetValues(w, args, 2);
    
    is_fg = (color_idx == app.state.foreground_color);
    is_bg = (color_idx == app.state.background_color);
    
    /* Fill with color */
    XSetForeground(app.display, app.ui_gc, app.pixels[color_idx]);
    XFillRectangle(app.display, win, app.ui_gc, 0, 0, width, height);
    
    /* Draw selection indicators */
    if (is_fg) {
        /* White border for foreground */
        XSetForeground(app.display, app.ui_gc, app.white_pixel);
        XSetLineAttributes(app.display, app.ui_gc, 2, LineSolid, CapButt, JoinMiter);
        XDrawRectangle(app.display, win, app.ui_gc, 1, 1, width - 3, height - 3);
    }
    
    if (is_bg) {
        /* Black inner border for background */
        XSetForeground(app.display, app.ui_gc, app.black_pixel);
        XSetLineAttributes(app.display, app.ui_gc, 1, LineSolid, CapButt, JoinMiter);
        XDrawRectangle(app.display, win, app.ui_gc, 3, 3, width - 7, height - 7);
    }
    
    /* Always draw outer black border */
    XSetForeground(app.display, app.ui_gc, app.black_pixel);
    XSetLineAttributes(app.display, app.ui_gc, 1, LineSolid, CapButt, JoinMiter);
    XDrawRectangle(app.display, win, app.ui_gc, 0, 0, width - 1, height - 1);
}

static void fgbg_expose_cb(Widget w, XtPointer client, XtPointer call)
{
    Window win;
    Dimension width, height;
    Arg args[4];
    int fg_x, fg_y, fg_w, fg_h;
    int bg_x, bg_y, bg_w, bg_h;
    XmDrawingAreaCallbackStruct* cbs = (XmDrawingAreaCallbackStruct*)call;
    
    if (!cbs || cbs->reason != XmCR_EXPOSE) return;
    if (!app.ui_gc) return;
    
    win = XtWindow(w);
    if (!win) return;
    
    XtSetArg(args[0], XmNwidth, &width);
    XtSetArg(args[1], XmNheight, &height);
    XtGetValues(w, args, 2);
    
    /* Clear background */
    XSetForeground(app.display, app.ui_gc, app.gray_pixel);
    XFillRectangle(app.display, win, app.ui_gc, 0, 0, width, height);
    
    /* Calculate positions - BG behind and offset, FG in front */
    fg_w = 24;
    fg_h = 24;
    bg_w = 24;
    bg_h = 24;
    
    bg_x = 18;
    bg_y = 14;
    fg_x = 6;
    fg_y = 4;
    
    /* Draw background color box (behind) */
    XSetForeground(app.display, app.ui_gc, app.pixels[app.state.background_color]);
    XFillRectangle(app.display, win, app.ui_gc, bg_x, bg_y, bg_w, bg_h);
    XSetForeground(app.display, app.ui_gc, app.black_pixel);
    XDrawRectangle(app.display, win, app.ui_gc, bg_x, bg_y, bg_w - 1, bg_h - 1);
    
    /* Draw foreground color box (in front) */
    XSetForeground(app.display, app.ui_gc, app.pixels[app.state.foreground_color]);
    XFillRectangle(app.display, win, app.ui_gc, fg_x, fg_y, fg_w, fg_h);
    XSetForeground(app.display, app.ui_gc, app.black_pixel);
    XDrawRectangle(app.display, win, app.ui_gc, fg_x, fg_y, fg_w - 1, fg_h - 1);
}

/* ========================================================================== */
/* Canvas Callbacks                                                           */
/* ========================================================================== */

static void canvas_expose_cb(Widget w, XtPointer client, XtPointer call)
{
    refresh_canvas();
}

static void canvas_input_cb(Widget w, XtPointer client, XtPointer call)
{
    XmDrawingAreaCallbackStruct* cbs = (XmDrawingAreaCallbackStruct*)call;
    XEvent* event = cbs->event;
    int x, y;
    
    if (event->type == ButtonPress) {
        x = event->xbutton.x;
        y = event->xbutton.y;
        
        if (event->xbutton.button == Button1) {
            on_mouse_down(x, y, 1);
        } else if (event->xbutton.button == Button3) {
            /* Right button - swap fg/bg temporarily for drawing */
            int temp = app.state.foreground_color;
            app.state.foreground_color = app.state.background_color;
            on_mouse_down(x, y, 3);
            if (app.state.current_tool != TOOL_PICKER) {
                app.state.foreground_color = temp;
            }
        }
    }
    else if (event->type == ButtonRelease) {
        x = event->xbutton.x;
        y = event->xbutton.y;
        on_mouse_up(x, y, event->xbutton.button);
    }
}

static void canvas_motion_eh(Widget w, XtPointer client, XEvent* event, Boolean* cont)
{
    if (event->type == MotionNotify) {
        on_mouse_move(event->xmotion.x, event->xmotion.y, event->xmotion.state);
    }
    *cont = True;
}

/* ========================================================================== */
/* File Dialog Callbacks                                                      */
/* ========================================================================== */

static void file_dialog_ok_cb(Widget w, XtPointer client, XtPointer call)
{
    XmFileSelectionBoxCallbackStruct* cbs = (XmFileSelectionBoxCallbackStruct*)call;
    char* filename;
    
    if (!XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename)) {
        return;
    }
    
    if (app.file_dialog_mode == 0) {
        /* Open mode */
        PixelBuffer* loaded;
        
        loaded = bmp_load(filename);
        if (!loaded) {
            Widget dialog;
            Arg args[5];
            int n;
            XmString str;
            
            n = 0;
            str = XmStringCreateSimple("Failed to open file.\nMake sure it's a valid 24-bit BMP file.");
            XtSetArg(args[n], XmNmessageString, str); n++;
            XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
            dialog = XmCreateErrorDialog(app.toplevel, "error", args, n);
            XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
            XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
            XtManageChild(dialog);
            XmStringFree(str);
            XtFree(filename);
            return;
        }
        
        /* Replace canvas */
        pixbuf_destroy(app.pb_canvas);
        app.pb_canvas = loaded;
        
        /* Update temp canvas */
        pixbuf_destroy(app.pb_temp);
        app.pb_temp = pixbuf_create(app.pb_canvas->width, app.pb_canvas->height);
        memcpy(app.pb_temp->data, app.pb_canvas->data,
               app.pb_canvas->row_stride * app.pb_canvas->height);
        
        /* Clear undo */
        undo_clear(app.undo);
        
        /* Update state */
        strncpy(app.state.filename, filename, sizeof(app.state.filename) - 1);
        app.state.filename[sizeof(app.state.filename) - 1] = '\0';
        app.state.modified = 0;
        
        sync_ximage_from_pixbuf();
        refresh_canvas();
        update_title();
    }
    else {
        /* Save mode */
        int len;
        
        strncpy(app.state.filename, filename, sizeof(app.state.filename) - 1);
        app.state.filename[sizeof(app.state.filename) - 1] = '\0';
        
        /* Ensure .bmp extension */
        len = strlen(app.state.filename);
        if (len < 4 || 
            (strcmp(app.state.filename + len - 4, ".bmp") != 0 &&
             strcmp(app.state.filename + len - 4, ".BMP") != 0)) {
            if (len < (int)sizeof(app.state.filename) - 5) {
                strcat(app.state.filename, ".bmp");
            }
        }
        
        do_file_save();
    }
    
    XtFree(filename);
    XtUnmanageChild(app.file_dialog);
}

static void file_dialog_cancel_cb(Widget w, XtPointer client, XtPointer call)
{
    XtUnmanageChild(app.file_dialog);
}