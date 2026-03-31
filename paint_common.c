/*
 * paint_common.c - Common paint functionality
 * Compatible with C89, MSVC 4.0 through VS2022, GCC 2.95+
 */

/* Suppress MSVC secure function warnings (fopen, sprintf, etc.) */
#define _CRT_SECURE_NO_WARNINGS
#define _MBCS

#include "paint.h"
#include <math.h>
#include <time.h>



/* 
 * BMP File Format Structures
 * Defined here to avoid header packing issues across compilers
 */


/* BMP file header - 14 bytes */
typedef struct {
    unsigned char  bfType[2];       /* "BM" */
    unsigned char  bfSize[4];       /* File size */
    unsigned char  bfReserved1[2];  /* Reserved */
    unsigned char  bfReserved2[2];  /* Reserved */
    unsigned char  bfOffBits[4];    /* Offset to pixel data */
} BmpFileHeader;

/* BMP info header - 40 bytes */
typedef struct {
    unsigned char  biSize[4];         /* Header size (40) */
    unsigned char  biWidth[4];        /* Image width */
    unsigned char  biHeight[4];       /* Image height */
    unsigned char  biPlanes[2];       /* Color planes (1) */
    unsigned char  biBitCount[2];     /* Bits per pixel */
    unsigned char  biCompression[4];  /* Compression type */
    unsigned char  biSizeImage[4];    /* Image data size */
    unsigned char  biXPelsPerMeter[4]; /* X pixels per meter */
    unsigned char  biYPelsPerMeter[4]; /* Y pixels per meter */
    unsigned char  biClrUsed[4];      /* Colors used */
    unsigned char  biClrImportant[4]; /* Important colors */
} BmpInfoHeader;

/* Helper functions to write little-endian values */
static void write_u16_le(unsigned char* p, unsigned int val)
{
    p[0] = (unsigned char)(val & 0xFF);
    p[1] = (unsigned char)((val >> 8) & 0xFF);
}

static void write_u32_le(unsigned char* p, unsigned long val)
{
    p[0] = (unsigned char)(val & 0xFF);
    p[1] = (unsigned char)((val >> 8) & 0xFF);
    p[2] = (unsigned char)((val >> 16) & 0xFF);
    p[3] = (unsigned char)((val >> 24) & 0xFF);
}

static unsigned int read_u16_le(const unsigned char* p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static unsigned long read_u32_le(const unsigned char* p)
{
    return (unsigned long)p[0] | 
           ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

static long read_s32_le(const unsigned char* p)
{
    unsigned long u = read_u32_le(p);
    return (long)u;
}

/* Global color palette */
PaintColor g_palette[MAX_COLORS];

/* Tool names */
const char* g_tool_names[MAX_TOOLS] = {
    "Pencil",
    "Brush",
    "Airbrush",
    "Line",
    "Rectangle",
    "Filled Rect",
    "Ellipse",
    "Filled Ellipse",
    "Round Rect",
    "Filled RRect",
    "Eraser",
    "Fill",
    "Color Picker"
};

/* Random number generator state */
static unsigned long g_rand_seed = 1;

/*
 * Utility functions
 */

int min_int(int a, int b) 
{ 
    return (a < b) ? a : b; 
}

int max_int(int a, int b) 
{ 
    return (a > b) ? a : b; 
}

int abs_int(int a) 
{ 
    return (a < 0) ? -a : a; 
}

void swap_int(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

int clamp_int(int val, int min_val, int max_val)
{
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

/* Simple random for airbrush */
static int paint_rand(void)
{
    g_rand_seed = g_rand_seed * 1103515245UL + 12345UL;
    return (int)((g_rand_seed >> 16) & 0x7FFF);
}

static void paint_srand(unsigned long seed)
{
    g_rand_seed = seed;
}

/*
 * Initialize color palette - matches classic MS Paint colors
 */
void paint_init_palette(void)
{
    /* Row 1: Basic colors */
    g_palette[0].r = 0;   g_palette[0].g = 0;   g_palette[0].b = 0;       /* Black */
    g_palette[1].r = 128; g_palette[1].g = 128; g_palette[1].b = 128;     /* Gray */
    g_palette[2].r = 128; g_palette[2].g = 0;   g_palette[2].b = 0;       /* Maroon */
    g_palette[3].r = 128; g_palette[3].g = 128; g_palette[3].b = 0;       /* Olive */
    g_palette[4].r = 0;   g_palette[4].g = 128; g_palette[4].b = 0;       /* Green */
    g_palette[5].r = 0;   g_palette[5].g = 128; g_palette[5].b = 128;     /* Teal */
    g_palette[6].r = 0;   g_palette[6].g = 0;   g_palette[6].b = 128;     /* Navy */
    g_palette[7].r = 128; g_palette[7].g = 0;   g_palette[7].b = 128;     /* Purple */
    g_palette[8].r = 128; g_palette[8].g = 128; g_palette[8].b = 64;      /* Dark Yellow */
    g_palette[9].r = 0;   g_palette[9].g = 64;  g_palette[9].b = 64;      /* Dark Cyan */
    g_palette[10].r = 0;  g_palette[10].g = 128; g_palette[10].b = 255;   /* Sky Blue */
    g_palette[11].r = 0;  g_palette[11].g = 64; g_palette[11].b = 128;    /* Dark Blue */
    g_palette[12].r = 128; g_palette[12].g = 0; g_palette[12].b = 255;    /* Violet */
    g_palette[13].r = 128; g_palette[13].g = 64; g_palette[13].b = 0;     /* Brown */
    
    /* Row 2: Bright colors */
    g_palette[14].r = 255; g_palette[14].g = 255; g_palette[14].b = 255;  /* White */
    g_palette[15].r = 192; g_palette[15].g = 192; g_palette[15].b = 192;  /* Light Gray */
    g_palette[16].r = 255; g_palette[16].g = 0;   g_palette[16].b = 0;    /* Red */
    g_palette[17].r = 255; g_palette[17].g = 255; g_palette[17].b = 0;    /* Yellow */
    g_palette[18].r = 0;   g_palette[18].g = 255; g_palette[18].b = 0;    /* Lime */
    g_palette[19].r = 0;   g_palette[19].g = 255; g_palette[19].b = 255;  /* Cyan */
    g_palette[20].r = 0;   g_palette[20].g = 0;   g_palette[20].b = 255;  /* Blue */
    g_palette[21].r = 255; g_palette[21].g = 0;   g_palette[21].b = 255;  /* Magenta */
    g_palette[22].r = 255; g_palette[22].g = 255; g_palette[22].b = 128;  /* Light Yellow */
    g_palette[23].r = 0;   g_palette[23].g = 255; g_palette[23].b = 128;  /* Light Green */
    g_palette[24].r = 128; g_palette[24].g = 255; g_palette[24].b = 255;  /* Light Cyan */
    g_palette[25].r = 128; g_palette[25].g = 128; g_palette[25].b = 255;  /* Light Blue */
    g_palette[26].r = 255; g_palette[26].g = 0;   g_palette[26].b = 128;  /* Pink */
    g_palette[27].r = 255; g_palette[27].g = 128; g_palette[27].b = 64;   /* Orange */
    
    /* Initialize random seed */
    paint_srand((unsigned long)time(NULL));
}

void paint_init_state(PaintState* state)
{
    state->current_tool = TOOL_PENCIL;
    state->foreground_color = 0;    /* Black */
    state->background_color = 14;   /* White */
    state->brush_size = BRUSH_SMALL;
    state->brush_shape = BRUSH_ROUND;
    state->is_drawing = 0;
    state->drawing_button = 1;
    state->start_point.x = 0;
    state->start_point.y = 0;
    state->last_point.x = 0;
    state->last_point.y = 0;
    state->current_point.x = 0;
    state->current_point.y = 0;
    state->filename[0] = '\0';
    state->modified = 0;
    state->airbrush_density = 50;
}

/*
 * Pixel Buffer Management
 */

PixelBuffer* pixbuf_create(int width, int height)
{
    PixelBuffer* pb;
    
    pb = (PixelBuffer*)malloc(sizeof(PixelBuffer));
    if (!pb) return NULL;
    
    pb->width = width;
    pb->height = height;
    pb->row_stride = width * 3;
    
    /* Align to 4-byte boundary for BMP compatibility */
    while (pb->row_stride % 4 != 0) {
        pb->row_stride++;
    }
    
    pb->data = (unsigned char*)malloc((size_t)pb->row_stride * (size_t)height);
    if (!pb->data) {
        free(pb);
        return NULL;
    }
    
    /* Initialize to white */
    memset(pb->data, 255, (size_t)pb->row_stride * (size_t)height);
    
    return pb;
}

void pixbuf_destroy(PixelBuffer* pb)
{
    if (pb) {
        if (pb->data) {
            free(pb->data);
        }
        free(pb);
    }
}

PixelBuffer* pixbuf_copy(const PixelBuffer* src)
{
    PixelBuffer* dst;
    
    if (!src) return NULL;
    
    dst = pixbuf_create(src->width, src->height);
    if (!dst) return NULL;
    
    memcpy(dst->data, src->data, (size_t)src->row_stride * (size_t)src->height);
    
    return dst;
}

void pixbuf_clear(PixelBuffer* pb, PaintColor color)
{
    int x, y;
    unsigned char* row;
    
    if (!pb || !pb->data) return;
    
    for (y = 0; y < pb->height; y++) {
        row = pb->data + y * pb->row_stride;
        for (x = 0; x < pb->width; x++) {
            row[x * 3 + 0] = color.r;
            row[x * 3 + 1] = color.g;
            row[x * 3 + 2] = color.b;
        }
    }
}

void pixbuf_set_pixel(PixelBuffer* pb, int x, int y, PaintColor color)
{
    unsigned char* pixel;
    
    if (!pb || !pb->data) return;
    if (x < 0 || x >= pb->width || y < 0 || y >= pb->height) return;
    
    pixel = pb->data + y * pb->row_stride + x * 3;
    pixel[0] = color.r;
    pixel[1] = color.g;
    pixel[2] = color.b;
}

PaintColor pixbuf_get_pixel(const PixelBuffer* pb, int x, int y)
{
    PaintColor c;
    const unsigned char* pixel;
    
    c.r = c.g = c.b = 0;
    
    if (!pb || !pb->data) return c;
    if (x < 0 || x >= pb->width || y < 0 || y >= pb->height) return c;
    
    pixel = pb->data + y * pb->row_stride + x * 3;
    c.r = pixel[0];
    c.g = pixel[1];
    c.b = pixel[2];
    
    return c;
}

/*
 * Drawing Primitives
 */

/* Bresenham's line algorithm with thickness */
void draw_line(PixelBuffer* pb, int x1, int y1, int x2, int y2,
               PaintColor color, int thickness)
{
    int dx, dy, sx, sy, err, e2;
    int half_thick;
    int i, j;
    
    if (!pb) return;
    
    dx = abs_int(x2 - x1);
    dy = abs_int(y2 - y1);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;
    
    half_thick = thickness / 2;
    
    while (1) {
        /* Draw thick point */
        if (thickness <= 1) {
            pixbuf_set_pixel(pb, x1, y1, color);
        } else {
            for (j = -half_thick; j <= half_thick; j++) {
                for (i = -half_thick; i <= half_thick; i++) {
                    /* Round brush */
                    if (i*i + j*j <= half_thick*half_thick + half_thick) {
                        pixbuf_set_pixel(pb, x1 + i, y1 + j, color);
                    }
                }
            }
        }
        
        if (x1 == x2 && y1 == y2) break;
        
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

/* Draw rectangle */
void draw_rect(PixelBuffer* pb, int x1, int y1, int x2, int y2,
               PaintColor color, int thickness, int filled)
{
    int x, y;
    
    if (!pb) return;
    
    /* Normalize coordinates */
    if (x1 > x2) swap_int(&x1, &x2);
    if (y1 > y2) swap_int(&y1, &y2);
    
    if (filled) {
        for (y = y1; y <= y2; y++) {
            for (x = x1; x <= x2; x++) {
                pixbuf_set_pixel(pb, x, y, color);
            }
        }
    } else {
        /* Draw four lines */
        draw_line(pb, x1, y1, x2, y1, color, thickness);  /* Top */
        draw_line(pb, x1, y2, x2, y2, color, thickness);  /* Bottom */
        draw_line(pb, x1, y1, x1, y2, color, thickness);  /* Left */
        draw_line(pb, x2, y1, x2, y2, color, thickness);  /* Right */
    }
}

/* Midpoint ellipse algorithm */
void draw_ellipse(PixelBuffer* pb, int x1, int y1, int x2, int y2,
                  PaintColor color, int thickness, int filled)
{
    int cx, cy, rx, ry;
    int x, y;
    int rx2, ry2;
    int tworx2, twory2;
    int p;
    int px, py;
    int half_thick;
    int i, j;
    double fy, fx;
    int x_extent;
    
    if (!pb) return;
    
    /* Normalize coordinates */
    if (x1 > x2) swap_int(&x1, &x2);
    if (y1 > y2) swap_int(&y1, &y2);
    
    /* Calculate center and radii */
    cx = (x1 + x2) / 2;
    cy = (y1 + y2) / 2;
    rx = (x2 - x1) / 2;
    ry = (y2 - y1) / 2;
    
    if (rx < 1) rx = 1;
    if (ry < 1) ry = 1;
    
    half_thick = thickness / 2;
    
    if (filled) {
        /* Filled ellipse using scanlines */
        for (y = -ry; y <= ry; y++) {
            fy = (double)y / (double)ry;
            fx = sqrt(1.0 - fy * fy);
            x_extent = (int)(fx * rx + 0.5);
            
            for (x = -x_extent; x <= x_extent; x++) {
                pixbuf_set_pixel(pb, cx + x, cy + y, color);
            }
        }
    } else {
        /* Outline ellipse using midpoint algorithm */
        rx2 = rx * rx;
        ry2 = ry * ry;
        tworx2 = 2 * rx2;
        twory2 = 2 * ry2;
        
        x = 0;
        y = ry;
        px = 0;
        py = tworx2 * y;
        
        /* Region 1 */
        p = (int)(ry2 - rx2 * ry + 0.25 * rx2);
        while (px < py) {
            /* Plot four quadrants with thickness */
            for (j = -half_thick; j <= half_thick; j++) {
                for (i = -half_thick; i <= half_thick; i++) {
                    if (thickness <= 1 || i*i + j*j <= half_thick*half_thick + half_thick) {
                        pixbuf_set_pixel(pb, cx + x + i, cy + y + j, color);
                        pixbuf_set_pixel(pb, cx - x + i, cy + y + j, color);
                        pixbuf_set_pixel(pb, cx + x + i, cy - y + j, color);
                        pixbuf_set_pixel(pb, cx - x + i, cy - y + j, color);
                    }
                }
            }
            
            x++;
            px += twory2;
            if (p < 0) {
                p += ry2 + px;
            } else {
                y--;
                py -= tworx2;
                p += ry2 + px - py;
            }
        }
        
        /* Region 2 */
        p = (int)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
        while (y >= 0) {
            for (j = -half_thick; j <= half_thick; j++) {
                for (i = -half_thick; i <= half_thick; i++) {
                    if (thickness <= 1 || i*i + j*j <= half_thick*half_thick + half_thick) {
                        pixbuf_set_pixel(pb, cx + x + i, cy + y + j, color);
                        pixbuf_set_pixel(pb, cx - x + i, cy + y + j, color);
                        pixbuf_set_pixel(pb, cx + x + i, cy - y + j, color);
                        pixbuf_set_pixel(pb, cx - x + i, cy - y + j, color);
                    }
                }
            }
            
            y--;
            py -= tworx2;
            if (p > 0) {
                p += rx2 - py;
            } else {
                x++;
                px += twory2;
                p += rx2 - py + px;
            }
        }
    }
}

/* Rounded rectangle */
void draw_round_rect(PixelBuffer* pb, int x1, int y1, int x2, int y2,
                     PaintColor color, int thickness, int filled, int radius)
{
    int x, y;
    int min_dim;
    int half_thick;
    int r2;
    int cx, cy;
    int dx, dy;
    double angle;
    int px, py;
    int t;
    int i, j;
    
    if (!pb) return;
    
    /* Normalize coordinates */
    if (x1 > x2) swap_int(&x1, &x2);
    if (y1 > y2) swap_int(&y1, &y2);
    
    /* Limit radius */
    min_dim = min_int(x2 - x1, y2 - y1) / 2;
    if (radius > min_dim) radius = min_dim;
    if (radius < 1) radius = 1;
    
    half_thick = thickness / 2;
    
    if (filled) {
        /* Fill center rectangle */
        for (y = y1 + radius; y <= y2 - radius; y++) {
            for (x = x1; x <= x2; x++) {
                pixbuf_set_pixel(pb, x, y, color);
            }
        }
        
        /* Fill top and bottom strips */
        for (y = y1; y < y1 + radius; y++) {
            for (x = x1 + radius; x <= x2 - radius; x++) {
                pixbuf_set_pixel(pb, x, y, color);
            }
        }
        for (y = y2 - radius + 1; y <= y2; y++) {
            for (x = x1 + radius; x <= x2 - radius; x++) {
                pixbuf_set_pixel(pb, x, y, color);
            }
        }
        
        /* Fill corners with quarter circles */
        r2 = radius * radius;
        
        /* Top-left corner */
        cx = x1 + radius;
        cy = y1 + radius;
        for (dy = 0; dy < radius; dy++) {
            for (dx = 0; dx < radius; dx++) {
                if (dx*dx + dy*dy <= r2) {
                    pixbuf_set_pixel(pb, cx - dx, cy - dy, color);
                }
            }
        }
        
        /* Top-right corner */
        cx = x2 - radius;
        cy = y1 + radius;
        for (dy = 0; dy < radius; dy++) {
            for (dx = 0; dx < radius; dx++) {
                if (dx*dx + dy*dy <= r2) {
                    pixbuf_set_pixel(pb, cx + dx, cy - dy, color);
                }
            }
        }
        
        /* Bottom-left corner */
        cx = x1 + radius;
        cy = y2 - radius;
        for (dy = 0; dy < radius; dy++) {
            for (dx = 0; dx < radius; dx++) {
                if (dx*dx + dy*dy <= r2) {
                    pixbuf_set_pixel(pb, cx - dx, cy + dy, color);
                }
            }
        }
        
        /* Bottom-right corner */
        cx = x2 - radius;
        cy = y2 - radius;
        for (dy = 0; dy < radius; dy++) {
            for (dx = 0; dx < radius; dx++) {
                if (dx*dx + dy*dy <= r2) {
                    pixbuf_set_pixel(pb, cx + dx, cy + dy, color);
                }
            }
        }
    } else {
        /* Draw outline */
        /* Horizontal lines */
        draw_line(pb, x1 + radius, y1, x2 - radius, y1, color, thickness);
        draw_line(pb, x1 + radius, y2, x2 - radius, y2, color, thickness);
        
        /* Vertical lines */
        draw_line(pb, x1, y1 + radius, x1, y2 - radius, color, thickness);
        draw_line(pb, x2, y1 + radius, x2, y2 - radius, color, thickness);
        
        /* Corner arcs */
        /* Top-left arc */
        cx = x1 + radius;
        cy = y1 + radius;
        for (t = 0; t <= 90; t += 2) {
            angle = t * 3.14159265 / 180.0;
            px = cx - (int)(cos(angle) * radius);
            py = cy - (int)(sin(angle) * radius);
            for (j = -half_thick; j <= half_thick; j++) {
                for (i = -half_thick; i <= half_thick; i++) {
                    if (thickness <= 1 || i*i + j*j <= half_thick*half_thick) {
                        pixbuf_set_pixel(pb, px + i, py + j, color);
                    }
                }
            }
        }
        
        /* Top-right arc */
        cx = x2 - radius;
        cy = y1 + radius;
        for (t = 0; t <= 90; t += 2) {
            angle = t * 3.14159265 / 180.0;
            px = cx + (int)(cos(angle) * radius);
            py = cy - (int)(sin(angle) * radius);
            for (j = -half_thick; j <= half_thick; j++) {
                for (i = -half_thick; i <= half_thick; i++) {
                    if (thickness <= 1 || i*i + j*j <= half_thick*half_thick) {
                        pixbuf_set_pixel(pb, px + i, py + j, color);
                    }
                }
            }
        }
        
        /* Bottom-left arc */
        cx = x1 + radius;
        cy = y2 - radius;
        for (t = 0; t <= 90; t += 2) {
            angle = t * 3.14159265 / 180.0;
            px = cx - (int)(cos(angle) * radius);
            py = cy + (int)(sin(angle) * radius);
            for (j = -half_thick; j <= half_thick; j++) {
                for (i = -half_thick; i <= half_thick; i++) {
                    if (thickness <= 1 || i*i + j*j <= half_thick*half_thick) {
                        pixbuf_set_pixel(pb, px + i, py + j, color);
                    }
                }
            }
        }
        
        /* Bottom-right arc */
        cx = x2 - radius;
        cy = y2 - radius;
        for (t = 0; t <= 90; t += 2) {
            angle = t * 3.14159265 / 180.0;
            px = cx + (int)(cos(angle) * radius);
            py = cy + (int)(sin(angle) * radius);
            for (j = -half_thick; j <= half_thick; j++) {
                for (i = -half_thick; i <= half_thick; i++) {
                    if (thickness <= 1 || i*i + j*j <= half_thick*half_thick) {
                        pixbuf_set_pixel(pb, px + i, py + j, color);
                    }
                }
            }
        }
    }
}

/* Brush stroke with shape */
void draw_brush_stroke(PixelBuffer* pb, int x, int y,
                       PaintColor color, int size, int shape)
{
    int half = size / 2;
    int i, j;
    
    if (!pb) return;
    
    switch (shape) {
    case BRUSH_SQUARE:
        for (j = -half; j <= half; j++) {
            for (i = -half; i <= half; i++) {
                pixbuf_set_pixel(pb, x + i, y + j, color);
            }
        }
        break;
        
    case BRUSH_ROUND:
    default:
        for (j = -half; j <= half; j++) {
            for (i = -half; i <= half; i++) {
                if (i*i + j*j <= half*half + half) {
                    pixbuf_set_pixel(pb, x + i, y + j, color);
                }
            }
        }
        break;
        
    case BRUSH_SLASH:
        for (i = -half; i <= half; i++) {
            pixbuf_set_pixel(pb, x + i, y - i, color);
            pixbuf_set_pixel(pb, x + i, y - i + 1, color);
        }
        break;
        
    case BRUSH_BACKSLASH:
        for (i = -half; i <= half; i++) {
            pixbuf_set_pixel(pb, x + i, y + i, color);
            pixbuf_set_pixel(pb, x + i, y + i + 1, color);
        }
        break;
    }
}

/* Airbrush effect - random spray */
void draw_airbrush(PixelBuffer* pb, int x, int y,
                   PaintColor color, int size, int density)
{
    int half = size / 2;
    int i;
    int num_dots;
    int dx, dy;
    
    if (!pb) return;
    
    /* Number of dots based on density and size */
    num_dots = (size * size * density) / 400;
    if (num_dots < 3) num_dots = 3;
    if (num_dots > 100) num_dots = 100;
    
    for (i = 0; i < num_dots; i++) {
        /* Random point within circle */
        do {
            dx = (paint_rand() % (size + 1)) - half;
            dy = (paint_rand() % (size + 1)) - half;
        } while (dx*dx + dy*dy > half*half);
        
        pixbuf_set_pixel(pb, x + dx, y + dy, color);
    }
}

/* Flood fill using stack-based algorithm */
void flood_fill_area(PixelBuffer* pb, int x, int y, PaintColor fill_color)
{
    PaintColor target_color;
    PaintColor current;
    int* stack_x;
    int* stack_y;
    int stack_size, stack_max;
    int cx, cy;
    
    if (!pb || !pb->data) return;
    if (x < 0 || x >= pb->width || y < 0 || y >= pb->height) return;
    
    target_color = pixbuf_get_pixel(pb, x, y);
    
    /* Don't fill if same color */
    if (target_color.r == fill_color.r &&
        target_color.g == fill_color.g &&
        target_color.b == fill_color.b) {
        return;
    }
    
    stack_max = pb->width * pb->height / 4;
    if (stack_max < 1000) stack_max = 1000;
    
    stack_x = (int*)malloc((size_t)stack_max * sizeof(int));
    stack_y = (int*)malloc((size_t)stack_max * sizeof(int));
    
    if (!stack_x || !stack_y) {
        if (stack_x) free(stack_x);
        if (stack_y) free(stack_y);
        return;
    }
    
    /* Push starting point */
    stack_size = 0;
    stack_x[stack_size] = x;
    stack_y[stack_size] = y;
    stack_size++;
    
    while (stack_size > 0) {
        /* Pop */
        stack_size--;
        cx = stack_x[stack_size];
        cy = stack_y[stack_size];
        
        if (cx < 0 || cx >= pb->width || cy < 0 || cy >= pb->height) {
            continue;
        }
        
        current = pixbuf_get_pixel(pb, cx, cy);
        
        if (current.r != target_color.r ||
            current.g != target_color.g ||
            current.b != target_color.b) {
            continue;
        }
        
        /* Fill this pixel */
        pixbuf_set_pixel(pb, cx, cy, fill_color);
        
        /* Push neighbors (4-connected) */
        if (stack_size + 4 < stack_max) {
            stack_x[stack_size] = cx + 1;
            stack_y[stack_size] = cy;
            stack_size++;
            
            stack_x[stack_size] = cx - 1;
            stack_y[stack_size] = cy;
            stack_size++;
            
            stack_x[stack_size] = cx;
            stack_y[stack_size] = cy + 1;
            stack_size++;
            
            stack_x[stack_size] = cx;
            stack_y[stack_size] = cy - 1;
            stack_size++;
        }
    }
    
    free(stack_x);
    free(stack_y);
}

/*
 * BMP File I/O - 24-bit uncompressed BMP
 */



int bmp_save(const char* filename, const PixelBuffer* pb)
{
    FILE* fp;
    BmpFileHeader file_hdr;
    BmpInfoHeader info_hdr;
    int y, x;
    int padding;
    unsigned char pad[4] = {0, 0, 0, 0};
    const unsigned char* row;
    unsigned char bgr[3];
    unsigned long file_size;
    unsigned long image_size;
    
    if (!filename || !pb || !pb->data) return 0;
    

    fp = fopen(filename, "wb");
    if (!fp) return 0;
    
    /* Calculate row padding for BMP (rows must be 4-byte aligned) */
    padding = (4 - ((pb->width * 3) % 4)) % 4;
    image_size = (unsigned long)(pb->width * 3 + padding) * (unsigned long)pb->height;
    file_size = 54 + image_size;
    
    /* Prepare file header */
    file_hdr.bfType[0] = 'B';
    file_hdr.bfType[1] = 'M';
    write_u32_le(file_hdr.bfSize, file_size);
    write_u16_le(file_hdr.bfReserved1, 0);
    write_u16_le(file_hdr.bfReserved2, 0);
    write_u32_le(file_hdr.bfOffBits, 54);
    
    /* Prepare info header */
    write_u32_le(info_hdr.biSize, 40);
    write_u32_le(info_hdr.biWidth, (unsigned long)pb->width);
    write_u32_le(info_hdr.biHeight, (unsigned long)pb->height);
    write_u16_le(info_hdr.biPlanes, 1);
    write_u16_le(info_hdr.biBitCount, 24);
    write_u32_le(info_hdr.biCompression, 0);
    write_u32_le(info_hdr.biSizeImage, image_size);
    write_u32_le(info_hdr.biXPelsPerMeter, 2835);  /* 72 DPI */
    write_u32_le(info_hdr.biYPelsPerMeter, 2835);
    write_u32_le(info_hdr.biClrUsed, 0);
    write_u32_le(info_hdr.biClrImportant, 0);
    
    /* Write file header (14 bytes) */
    fwrite(&file_hdr, 1, 14, fp);
    
    /* Write info header (40 bytes) */
    fwrite(&info_hdr, 1, 40, fp);
    
    /* Write pixel data (bottom-up, BGR format) */
    for (y = pb->height - 1; y >= 0; y--) {
        row = pb->data + y * pb->row_stride;
        for (x = 0; x < pb->width; x++) {
            /* Convert RGB to BGR */
            bgr[0] = row[x * 3 + 2];  /* B */
            bgr[1] = row[x * 3 + 1];  /* G */
            bgr[2] = row[x * 3 + 0];  /* R */
            fwrite(bgr, 1, 3, fp);
        }
        /* Write padding */
        if (padding > 0) {
            fwrite(pad, 1, (size_t)padding, fp);
        }
    }
    
    fclose(fp);
    return 1;
}

PixelBuffer* bmp_load(const char* filename)
{
    FILE* fp;
    PixelBuffer* pb;
    unsigned char header[54];
    int width, height, bpp;
    int padding;
    int y, x;
    unsigned char* row;
    unsigned char bgr[3];
    long offset;
    int compression;
    int top_down;
    int y_start, y_end, y_step;
    
    if (!filename) return NULL;
    
    fp = fopen(filename, "rb");
    if (!fp) return NULL;
    
    /* Read header */
    if (fread(header, 1, 54, fp) != 54) {
        fclose(fp);
        return NULL;
    }
    
    /* Check signature */
    if (header[0] != 'B' || header[1] != 'M') {
        fclose(fp);
        return NULL;
    }
    
    /* Extract dimensions */
    offset = (long)read_u32_le(&header[10]);
    width = (int)read_s32_le(&header[18]);
    height = (int)read_s32_le(&header[22]);
    bpp = (int)read_u16_le(&header[28]);
    compression = (int)read_u32_le(&header[30]);
    
    /* Handle negative height (top-down) */
    top_down = 0;
    if (height < 0) {
        height = -height;
        top_down = 1;
    }
    
    /* Only support 24-bit uncompressed */
    if (bpp != 24 || compression != 0) {
        fclose(fp);
        return NULL;
    }
    
    /* Sanity check dimensions */
    if (width <= 0 || width > 10000 || height <= 0 || height > 10000) {
        fclose(fp);
        return NULL;
    }
    
    /* Create pixel buffer */
    pb = pixbuf_create(width, height);
    if (!pb) {
        fclose(fp);
        return NULL;
    }
    
    /* Seek to pixel data */
    fseek(fp, offset, SEEK_SET);
    
    /* Calculate row padding */
    padding = (4 - ((width * 3) % 4)) % 4;
    
    /* Set up iteration based on top-down or bottom-up */
    if (top_down) {
        y_start = 0;
        y_end = height;
        y_step = 1;
    } else {
        y_start = height - 1;
        y_end = -1;
        y_step = -1;
    }
    
    /* Read pixel data */
    for (y = y_start; y != y_end; y += y_step) {
        row = pb->data + y * pb->row_stride;
        for (x = 0; x < width; x++) {
            if (fread(bgr, 1, 3, fp) != 3) {
                pixbuf_destroy(pb);
                fclose(fp);
                return NULL;
            }
            /* Convert BGR to RGB */
            row[x * 3 + 0] = bgr[2];  /* R */
            row[x * 3 + 1] = bgr[1];  /* G */
            row[x * 3 + 2] = bgr[0];  /* B */
        }
        /* Skip padding */
        if (padding > 0) {
            fseek(fp, padding, SEEK_CUR);
        }
    }
    
    fclose(fp);
    return pb;
}

/*
 * Undo Management
 */

UndoStack* undo_create(void)
{
    UndoStack* stack;
    int i;
    
    stack = (UndoStack*)malloc(sizeof(UndoStack));
    if (!stack) return NULL;
    
    for (i = 0; i < MAX_UNDO; i++) {
        stack->buffers[i] = NULL;
    }
    stack->count = 0;
    stack->current = -1;
    
    return stack;
}

void undo_destroy(UndoStack* stack)
{
    if (stack) {
        undo_clear(stack);
        free(stack);
    }
}

void undo_clear(UndoStack* stack)
{
    int i;
    
    if (!stack) return;
    
    for (i = 0; i < MAX_UNDO; i++) {
        if (stack->buffers[i]) {
            pixbuf_destroy(stack->buffers[i]);
            stack->buffers[i] = NULL;
        }
    }
    stack->count = 0;
    stack->current = -1;
}

void undo_push(UndoStack* stack, const PixelBuffer* pb)
{
    int next;
    
    if (!stack || !pb) return;
    
    /* Calculate next position */
    next = (stack->current + 1) % MAX_UNDO;
    
    /* Free old buffer at this position if exists */
    if (stack->buffers[next]) {
        pixbuf_destroy(stack->buffers[next]);
    }
    
    /* Copy current state */
    stack->buffers[next] = pixbuf_copy(pb);
    stack->current = next;
    
    if (stack->count < MAX_UNDO) {
        stack->count++;
    }
}

PixelBuffer* undo_pop(UndoStack* stack)
{
    PixelBuffer* result;
    int prev;
    
    if (!stack || stack->count <= 0 || stack->current < 0) {
        return NULL;
    }
    
    /* Get current state */
    result = stack->buffers[stack->current];
    stack->buffers[stack->current] = NULL;
    
    /* Move to previous */
    prev = stack->current - 1;
    if (prev < 0) prev = MAX_UNDO - 1;
    stack->current = prev;
    stack->count--;
    
    return result;
}

int undo_can_undo(const UndoStack* stack)
{
    if (!stack) return 0;
    return (stack->count > 0);
}