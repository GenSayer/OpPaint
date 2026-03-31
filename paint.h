/*
 * paint.h - Portable Paint Program Header
 * Compatible with C89, MSVC 4.0 through VS2022, GCC 2.95+
 */

#ifndef PAINT_H
#define PAINT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Ensure we have size_t */
#include <stddef.h>

/* Tool definitions */
#define TOOL_PENCIL 0
#define TOOL_BRUSH 1
#define TOOL_AIRBRUSH 2
#define TOOL_LINE 3
#define TOOL_RECT 4
#define TOOL_FILLRECT 5
#define TOOL_ELLIPSE 6
#define TOOL_FILLELLIPSE 7
#define TOOL_ROUNDRECT 8
#define TOOL_FILLROUNDRECT 9
#define TOOL_ERASER 10
#define TOOL_FILL 11
#define TOOL_PICKER 12

#define MAX_TOOLS 13

/* Color palette - 28 colors (like classic Paint) */
#define MAX_COLORS 28

/* Canvas size */
#define CANVAS_WIDTH 800
#define CANVAS_HEIGHT 600

/* Toolbar dimensions */
#define TOOLBAR_WIDTH 80
#define TOOL_BTN_SIZE 28
#define COLOR_BTN_SIZE 16

/* Brush sizes */
#define BRUSH_TINY 1
#define BRUSH_SMALL 3
#define BRUSH_MEDIUM 7
#define BRUSH_LARGE 15
#define BRUSH_XLARGE 25

/* Undo levels */
#define MAX_UNDO 10

/* Filename max length */
#define MAX_FILENAME 260

/* RGB color structure */
typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} PaintColor;

/* Point structure */
typedef struct {
  int x;
  int y;
} PaintPoint;

/* Rectangle structure */
typedef struct {
  int x;
  int y;
  int width;
  int height;
} PaintRect;

/* Canvas pixel buffer */
typedef struct {
  unsigned char *data; /* RGB data, 3 bytes per pixel */
  int width;
  int height;
  int row_stride; /* Bytes per row (may include padding) */
} PixelBuffer;

/* Undo buffer */
typedef struct {
  PixelBuffer *buffers[MAX_UNDO];
  int count;
  int current;
} UndoStack;

/* Application state */
typedef struct {
  int current_tool;
  int foreground_color;
  int background_color;
  int brush_size;
  int is_drawing;
  int drawing_button; /* 1=left (foreground), 2=right (background) */
  int brush_shape; /* 0=square, 1=round */
  PaintPoint start_point;
  PaintPoint last_point;
  PaintPoint current_point;

  /* File state */
  char filename[MAX_FILENAME];
  int modified;

  /* Airbrush settings */
  int airbrush_density;
} PaintState;

/* Brush shapes */
#define BRUSH_SQUARE 0
#define BRUSH_ROUND 1
#define BRUSH_SLASH 2
#define BRUSH_BACKSLASH 3

/* Color palette (defined in paint_common.c) */
extern PaintColor g_palette[MAX_COLORS];

/* Tool names */
extern const char *g_tool_names[MAX_TOOLS];

/* Common functions - State management */
void paint_init_state(PaintState *state);
void paint_init_palette(void);

/* Common functions - Pixel buffer management */
PixelBuffer *pixbuf_create(int width, int height);
void pixbuf_destroy(PixelBuffer *pb);
PixelBuffer *pixbuf_copy(const PixelBuffer *src);
void pixbuf_clear(PixelBuffer *pb, PaintColor color);
void pixbuf_set_pixel(PixelBuffer *pb, int x, int y, PaintColor color);
PaintColor pixbuf_get_pixel(const PixelBuffer *pb, int x, int y);

/* Common functions - Drawing primitives */
void draw_line(PixelBuffer *pb, int x1, int y1, int x2, int y2,
               PaintColor color, int thickness);
void draw_rect(PixelBuffer *pb, int x1, int y1, int x2, int y2,
               PaintColor color, int thickness, int filled);
void draw_ellipse(PixelBuffer *pb, int x1, int y1, int x2, int y2,
                  PaintColor color, int thickness, int filled);
void draw_round_rect(PixelBuffer *pb, int x1, int y1, int x2, int y2,
                     PaintColor color, int thickness, int filled, int radius);
void draw_brush_stroke(PixelBuffer *pb, int x, int y, PaintColor color,
                       int size, int shape);
void draw_airbrush(PixelBuffer *pb, int x, int y, PaintColor color, int size,
                   int density);
void flood_fill_area(PixelBuffer *pb, int x, int y, PaintColor fill_color);

/* Common functions - File I/O */
int bmp_save(const char *filename, const PixelBuffer *pb);
PixelBuffer *bmp_load(const char *filename);

/* Common functions - Undo management */
UndoStack *undo_create(void);
void undo_destroy(UndoStack *stack);
void undo_push(UndoStack *stack, const PixelBuffer *pb);
PixelBuffer *undo_pop(UndoStack *stack);
int undo_can_undo(const UndoStack *stack);
void undo_clear(UndoStack *stack);

/* Utility functions */
int min_int(int a, int b);
int max_int(int a, int b);
int abs_int(int a);
void swap_int(int *a, int *b);
int clamp_int(int val, int min_val, int max_val);

#endif /* PAINT_H */