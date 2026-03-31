/*
 * paint_win32.c - Win32 Paint Program
 * Compatible with C89, MSVC 4.0 through VS2022
 */

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _MBCS
/* Force ANSI build to maintain compatibility with narrow strings */
#undef UNICODE
#undef _UNICODE
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "paint.h"



/*
 * MSVC Version Detection:
 * MSVC4.0  = _MSC_VER 1000    MSVC4.2  = _MSC_VER 1020
 * MSVC5.0  = _MSC_VER 1100    MSVC6.0  = _MSC_VER 1200
 * VS2002  = _MSC_VER 1300     VS2003  = _MSC_VER 1310
 * VS2005  = _MSC_VER 1400     VS2008  = _MSC_VER 1500
 * VS2010  = _MSC_VER 1600     VS2012  = _MSC_VER 1700
 * VS2013  = _MSC_VER 1800     VS2015  = _MSC_VER 1900
 * VS2017  = _MSC_VER 1910-1916
 * VS2019  = _MSC_VER 1920-1929
 * VS2022  = _MSC_VER 1930+
 */

/* UINT_PTR compatibility - not available before MSVC6/Windows SDK updates */
#if defined(_MSC_VER) && (_MSC_VER < 1200)
    #ifndef UINT_PTR
        #define UINT_PTR UINT
    #endif
    #ifndef INT_PTR
        #define INT_PTR INT
    #endif
    #ifndef LONG_PTR
        #define LONG_PTR LONG
    #endif
#endif

/* Suppress MSVC warnings - version-specific */
#ifdef _MSC_VER
    #if _MSC_VER >= 1400
        /* VS2005+: More warnings available */
        #pragma warning(disable: 4100)  /* unreferenced formal parameter */
        #pragma warning(disable: 4244)  /* conversion, possible loss of data */
        #pragma warning(disable: 4996)  /* deprecated functions */
        #pragma warning(disable: 4267)  /* size_t to int conversion (64-bit) */
    #elif _MSC_VER >= 1200
        /* MSVC6/VS2002/VS2003 */
        #pragma warning(disable: 4100)
        #pragma warning(disable: 4244)
        #pragma warning(disable: 4996)
    #elif _MSC_VER >= 1000
        /* MSVC4.x: C4996 doesn't exist */
        #pragma warning(disable: 4100)
        #pragma warning(disable: 4244)
    #endif
#endif

/* Menu IDs */
#define IDM_FILE_NEW 101
#define IDM_FILE_OPEN 102
#define IDM_FILE_SAVE 103
#define IDM_FILE_SAVEAS 104
#define IDM_FILE_EXIT 105
#define IDM_EDIT_UNDO 201
#define IDM_EDIT_CLEAR 202
#define IDM_HELP_ABOUT 401

/* Control IDs */
#define IDB_TOOL_BASE 1000
#define IDB_COLOR_BASE 1100
#define IDB_SIZE_BASE 1200
#define IDC_STATUS 1400

/* Brush sizes */
static const int g_brush_sizes[5] = {1, 3, 7, 15, 25};

/* Global variables */
static HINSTANCE g_hInst;
static HWND g_hWndMain;
static HWND g_hWndCanvas;
static HWND g_hWndStatus;
static HWND g_hToolButtons[MAX_TOOLS];
static HWND g_hColorButtons[MAX_COLORS];
static HWND g_hSizeButtons[5];

static PixelBuffer *g_canvas = NULL;
static PixelBuffer *g_temp_canvas = NULL;
static UndoStack *g_undo = NULL;
static HDC g_hdcBuffer = NULL;
static HBITMAP g_hbmBuffer = NULL;
static HBITMAP g_hbmOld = NULL;

static PaintState g_state;
static COLORREF g_colors[MAX_COLORS];

static int g_selected_size_index = 1;
static int g_is_nt_351 = 0;
static HINSTANCE hCtl3d = NULL;

/* Function prototypes */
static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK CanvasWndProc(HWND, UINT, WPARAM, LPARAM);

static void CreateMainMenu(HWND hWnd);
static void CreateToolPanel(HWND hWnd);
static void CreateColorPanel(HWND hWnd);
static void CreateCanvasWindow(HWND hWnd);
static void CreateStatusBar(HWND hWnd);

static void InitializeColors(void);
static void InitializeCanvas(void);
static void CleanupCanvas(void);
static void SyncBufferFromPixbuf(void);
static void RefreshCanvas(void);

static void DoFileNew(void);
static void DoFileOpen(void);
static void DoFileSave(void);
static void DoFileSaveAs(void);
static void DoUndo(void);
static void DoClear(void);
static int ConfirmSave(void);

static void UpdateTitle(void);
static void UpdateStatusBar(void);
static void HighlightSelectedTool(void);
static void HighlightSelectedSize(void);

static void OnMouseDown(int x, int y, int button);
static void OnMouseMove(int x, int y);
static void OnMouseUp(int x, int y, int button);
static void DrawCurrentTool(int x1, int y1, int x2, int y2, int to_temp);

static void DrawColorButton(HDC hdc, int index, RECT *pRect, int is_fg,
                            int is_bg);
static void DrawFgBgDisplay(HWND hWnd, HDC hdc);

/* borrowed from https://bearwindows.zcm.com.au/winnt351.htm#20 */
void WINAPI Ctl3dLoad(HINSTANCE hInst)
{
  BOOL (WINAPI *Ctl3dRegister)(HANDLE);
  BOOL (WINAPI *Ctl3dAutoSubclass)(HANDLE);

  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
  hCtl3d = LoadLibrary(TEXT("ctl3d32.dll"));
  if ((UINT_PTR)hCtl3d <= 32) /* HINSTANCE_ERROR is 32, but cast needed for strictness */
    hCtl3d = LoadLibrary(TEXT("ctl3dv2.dll"));
  if ((UINT_PTR)hCtl3d <= 32)
    hCtl3d = LoadLibrary(TEXT("ctl3d.dll"));

  if ((UINT_PTR)hCtl3d <= 32)
  {
    hCtl3d = 0;
    return;
  }	

  Ctl3dRegister = (BOOL (WINAPI *)(HANDLE))GetProcAddress(hCtl3d, "Ctl3dRegister");
  Ctl3dAutoSubclass = (BOOL (WINAPI *)(HANDLE))GetProcAddress(hCtl3d, "Ctl3dAutoSubclass");

  if (Ctl3dRegister && Ctl3dAutoSubclass)
  {
    Ctl3dRegister(hInst);
    Ctl3dAutoSubclass(hInst);
 }

 SetErrorMode(0);
}

void WINAPI Ctl3dUnload(HINSTANCE hInst)
{
 BOOL (WINAPI *Ctl3dUnRegister)(HANDLE);

  if (hCtl3d)
  {
    Ctl3dUnRegister = (BOOL (WINAPI *)(HANDLE))GetProcAddress(hCtl3d, "Ctl3dUnRegister");
    if (Ctl3dUnRegister)
      Ctl3dUnRegister(hInst);

    FreeLibrary(hCtl3d);
    hCtl3d = NULL;
  }
}

/* Entry point */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASS wc;
  MSG msg;

  (void)hPrevInstance;
  (void)lpCmdLine;

  g_hInst = hInstance;

  /* Initialize common state */
  paint_init_palette();
  paint_init_state(&g_state);
  InitializeColors();

  /* Try to load CTL3D for better aesthetics on NT 3.51/Win 3.x */
  Ctl3dLoad(hInstance);

  /* Set default brush size */
  g_state.brush_size = g_brush_sizes[g_selected_size_index];

  /* Register main window class */
  memset(&wc, 0, sizeof(wc));
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = MainWndProc;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  wc.lpszClassName = "PaintMainClass";

  if (!RegisterClass(&wc)) {
    MessageBox(NULL, "Window Registration Failed!", "Error",
               MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  /* Register canvas class */
  wc.lpfnWndProc = CanvasWndProc;
  wc.hCursor = LoadCursor(NULL, IDC_CROSS);
  wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszClassName = "PaintCanvasClass";

  if (!RegisterClass(&wc)) {
    MessageBox(NULL, "Canvas Registration Failed!", "Error",
               MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  /* Create main window */
  g_hWndMain = CreateWindowEx(0, "PaintMainClass", "Untitled - Paint",
                              WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT, CW_USEDEFAULT, CANVAS_WIDTH + 120,
                              CANVAS_HEIGHT + 180, NULL, NULL, hInstance, NULL);

  if (!g_hWndMain) {
    MessageBox(NULL, "Window Creation Failed!", "Error",
               MB_ICONEXCLAMATION | MB_OK);
    return 0;
  }

  ShowWindow(g_hWndMain, nCmdShow);
  UpdateWindow(g_hWndMain);

  /* Message loop */
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  Ctl3dUnload(hInstance);

  return (int)msg.wParam;
}

static void InitializeColors(void) {
  int i;
  for (i = 0; i < MAX_COLORS; i++) {
    g_colors[i] = RGB(g_palette[i].r, g_palette[i].g, g_palette[i].b);
  }
}

static void CreateMainMenu(HWND hWnd) {
  HMENU hMenu, hSubMenu;

  hMenu = CreateMenu();

  /* File menu */
  hSubMenu = CreatePopupMenu();
  AppendMenu(hSubMenu, MF_STRING, IDM_FILE_NEW, "&New\tCtrl+N");
  AppendMenu(hSubMenu, MF_STRING, IDM_FILE_OPEN, "&Open...\tCtrl+O");
  AppendMenu(hSubMenu, MF_STRING, IDM_FILE_SAVE, "&Save\tCtrl+S");
  AppendMenu(hSubMenu, MF_STRING, IDM_FILE_SAVEAS, "Save &As...");
  AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hSubMenu, MF_STRING, IDM_FILE_EXIT, "E&xit\tAlt+F4");
  AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, "&File");

  /* Edit menu */
  hSubMenu = CreatePopupMenu();
  AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_UNDO, "&Undo\tCtrl+Z");
  AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
  AppendMenu(hSubMenu, MF_STRING, IDM_EDIT_CLEAR, "&Clear All");
  AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, "&Edit");

  /* Help menu */
  hSubMenu = CreatePopupMenu();
  AppendMenu(hSubMenu, MF_STRING, IDM_HELP_ABOUT, "&About...");
  AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, "&Help");

  SetMenu(hWnd, hMenu);
}

static void Draw3DButton(HDC hdc, RECT *rc, const char *text, int is_pushed) {
  HBRUSH hBrush;
  HPEN hPenHighlight, hPenShadow, hPenFrame, hOldPen;
  RECT rcFill = *rc;
  HFONT hFont;
  int oldBkMode;

  /* Colors */
  DWORD cFace = GetSysColor(COLOR_BTNFACE);
  DWORD cHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
  DWORD cShadow = GetSysColor(COLOR_BTNSHADOW);
  DWORD cFrame = GetSysColor(COLOR_WINDOWFRAME); /* or COLOR_BTNTEXT */
  DWORD cText = GetSysColor(COLOR_BTNTEXT);

  /* Fill background */
  hBrush = CreateSolidBrush(cFace);
  FillRect(hdc, rc, hBrush);
  DeleteObject(hBrush);

  /* Draw 3D borders manually (safe for NT 3.51) */
  hPenHighlight = CreatePen(PS_SOLID, 1, cHighlight);
  hPenShadow = CreatePen(PS_SOLID, 1, cShadow);
  hPenFrame = CreatePen(PS_SOLID, 1, cFrame);

  if (is_pushed) {
    /* Sunken look */
    /* Outer Frame/Shadow (Top/Left) */
    hOldPen = (HPEN)SelectObject(hdc, hPenShadow);
    MoveToEx(hdc, rc->left, rc->bottom - 1, NULL);
    LineTo(hdc, rc->left, rc->top);
    LineTo(hdc, rc->right, rc->top);

    /* Inner Shadow (Top/Left) */
    SelectObject(hdc, hPenFrame); /* Use frame/black for deeper shadow if desired, or shadow */
    /* Usually standard is: Shadow, then Black? Or just Shadow. 
       Let's stick to simple 1-pixel borders for Win 3.x look or 2-pixel for 95 */
     
    /* For "checked" state (Pushed in), generally:
       Top/Left: Shadow
       Bottom/Right: Highlight (sometimes) or just flat?
       Let's do: Top/Left = Shadow, Bottom/Right = Highlight (inverted raised)
    */
    
    /* Highlight (Bottom/Right) */
    SelectObject(hdc, hPenHighlight);
    LineTo(hdc, rc->right - 1, rc->bottom - 1);
    LineTo(hdc, rc->left, rc->bottom - 1);

    /* Text Offset */
    rcFill.left += 1;
    rcFill.top += 1;
  } else {
    /* Raised look */
    /* Highlight (Top/Left) */
    hOldPen = (HPEN)SelectObject(hdc, hPenHighlight);
    MoveToEx(hdc, rc->left, rc->bottom - 2, NULL);
    LineTo(hdc, rc->left, rc->top);
    LineTo(hdc, rc->right - 1, rc->top);

    /* Shadow (Bottom/Right) */
    SelectObject(hdc, hPenShadow);
    LineTo(hdc, rc->right - 1, rc->bottom - 1);
    LineTo(hdc, rc->left, rc->bottom - 1);
    
    /* Frame (Extreme right/bottom) usually black */
    SelectObject(hdc, hPenFrame);
    MoveToEx(hdc, rc->right - 1, rc->top, NULL);
    LineTo(hdc, rc->right - 1, rc->bottom - 1);
    LineTo(hdc, rc->left - 1, rc->bottom - 1); /* Overlap? Care needed */
    
    /* Let's simplify to standard Win 3.1 style button:
       White Top/Left
       Black Bottom/Right
       Dark Gray Inner Bottom/Right
    */
    SelectObject(hdc, hPenHighlight);
    MoveToEx(hdc, rc->left, rc->bottom-1, NULL);
    LineTo(hdc, rc->left, rc->top);
    LineTo(hdc, rc->right, rc->top);

    SelectObject(hdc, hPenFrame); /* Black */
    LineTo(hdc, rc->right-1, rc->bottom-1);
    LineTo(hdc, rc->left, rc->bottom-1);
    
    SelectObject(hdc, hPenShadow); /* Dark Gray */
    MoveToEx(hdc, rc->right-2, rc->top+1, NULL);
    LineTo(hdc, rc->right-2, rc->bottom-2);
    LineTo(hdc, rc->left+1, rc->bottom-2);
  }

  SelectObject(hdc, hOldPen);
  DeleteObject(hPenHighlight);
  DeleteObject(hPenShadow);
  DeleteObject(hPenFrame);

  /* Draw Text */
  oldBkMode = SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, cText);
  hFont = (HFONT)SendMessage(WindowFromDC(hdc), WM_GETFONT, 0, 0);
  if(!hFont) hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  
  SelectObject(hdc, hFont);
  DrawText(hdc, text, -1, &rcFill, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  SetBkMode(hdc, oldBkMode);
}

static void CreateToolPanel(HWND hWnd) {
  int i, y;
  HWND hLabel;
  HFONT hFont;
  static const char *tool_labels[MAX_TOOLS] = {
      "Pencil",   "Brush",   "Airbrush", "Line",    "Rect",
      "FillRect", "Ellipse", "FillElps", "RndRect", "FillRnd",
      "Eraser",   "Fill",    "Picker"};
  static const char *size_labels[5] = {"1", "3", "7", "15", "25"};

  /* Get default GUI font */
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

  y = 5;

  /* Tools label */
  hLabel = CreateWindow("STATIC", "Tools:", WS_CHILD | WS_VISIBLE | SS_LEFT, 5,
                        y, 80, 16, hWnd, NULL, g_hInst, NULL);
  SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
  y += 18;

  /* Detect NT 3.51 */
  {
      OSVERSIONINFO osvi;
      osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
      GetVersionEx(&osvi);
      if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 3 && osvi.dwMinorVersion == 51) {
          g_is_nt_351 = 1;
      }
  }

  /* Create tool buttons directly on main window */
  for (i = 0; i < MAX_TOOLS; i++) {
    DWORD style = WS_CHILD | WS_VISIBLE;
    if (g_is_nt_351) {
        style |= BS_OWNERDRAW;
    } else {
        style |= BS_RADIOBUTTON | BS_PUSHLIKE;
    }
    g_hToolButtons[i] = CreateWindow(
        "BUTTON", tool_labels[i],
        style, 5, y, 80, 22,
        hWnd, (HMENU)(UINT_PTR)(IDB_TOOL_BASE + i), g_hInst, NULL);
    SendMessage(g_hToolButtons[i], WM_SETFONT, (WPARAM)hFont, TRUE);
    y += 23;
  }

  y += 8;

  /* Size label */
  hLabel =
      CreateWindow("STATIC", "Brush Size:", WS_CHILD | WS_VISIBLE | SS_LEFT, 5,
                   y, 80, 16, hWnd, NULL, g_hInst, NULL);
  SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
  y += 18;

  /* Size buttons */
  for (i = 0; i < 5; i++) {
    DWORD style = WS_CHILD | WS_VISIBLE;
    if (g_is_nt_351) {
        style |= BS_OWNERDRAW;
    } else {
        style |= BS_RADIOBUTTON | BS_PUSHLIKE;
    }
    g_hSizeButtons[i] = CreateWindow(
        "BUTTON", size_labels[i],
        style, 5 + i * 17, y, 16,
        20, hWnd, (HMENU)(UINT_PTR)(IDB_SIZE_BASE + i), g_hInst, NULL);
    SendMessage(g_hSizeButtons[i], WM_SETFONT, (WPARAM)hFont, TRUE);
  }

  /* Set initial selection */
  HighlightSelectedTool();
  HighlightSelectedSize();
}

static void CreateColorPanel(HWND hWnd) {
  int i, row, col, x, y;
  int baseX, baseY;
  HFONT hFont;
  HWND hLabel;

  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

  /* Position color panel below canvas */
  baseX = 95;
  baseY = CANVAS_HEIGHT + 10;

  /* Colors label */
  hLabel = CreateWindow("STATIC",
                        "Colors (L=FG, R=BG):", WS_CHILD | WS_VISIBLE | SS_LEFT,
                        baseX, baseY, 150, 16, hWnd, NULL, g_hInst, NULL);
  SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

  baseY += 18;

  /* Color buttons - 2 rows of 14 colors each */
  for (i = 0; i < MAX_COLORS; i++) {
    row = i / 14;
    col = i % 14;
    x = baseX + col * (COLOR_BTN_SIZE + 2);
    y = baseY + row * (COLOR_BTN_SIZE + 2);

    /* Use regular buttons, we'll custom paint them */
    g_hColorButtons[i] =
        CreateWindow("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, x, y,
                     COLOR_BTN_SIZE, COLOR_BTN_SIZE, hWnd,
                     (HMENU)(UINT_PTR)(IDB_COLOR_BASE + i), g_hInst, NULL);
  }
}

static void CreateCanvasWindow(HWND hWnd) {
  g_hWndCanvas = CreateWindowEx(WS_EX_CLIENTEDGE, "PaintCanvasClass", "",
                                WS_CHILD | WS_VISIBLE, 90, 5, CANVAS_WIDTH,
                                CANVAS_HEIGHT, hWnd, NULL, g_hInst, NULL);
}

static void CreateStatusBar(HWND hWnd) {
  HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

  g_hWndStatus = CreateWindowEx(WS_EX_STATICEDGE, "STATIC", "Ready",
                                WS_CHILD | WS_VISIBLE | SS_LEFT, 5,
                                CANVAS_HEIGHT + 70, CANVAS_WIDTH + 85, 22, hWnd,
                                (HMENU)IDC_STATUS, g_hInst, NULL);
  SendMessage(g_hWndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
}

static void InitializeCanvas(void) {
  HDC hdc;
  PaintColor white;

  g_canvas = pixbuf_create(CANVAS_WIDTH, CANVAS_HEIGHT);
  g_temp_canvas = pixbuf_create(CANVAS_WIDTH, CANVAS_HEIGHT);
  g_undo = undo_create();

  white.r = 255;
  white.g = 255;
  white.b = 255;
  pixbuf_clear(g_canvas, white);
  pixbuf_clear(g_temp_canvas, white);

  hdc = GetDC(g_hWndCanvas);
  g_hdcBuffer = CreateCompatibleDC(hdc);
  g_hbmBuffer = CreateCompatibleBitmap(hdc, CANVAS_WIDTH, CANVAS_HEIGHT);
  g_hbmOld = (HBITMAP)SelectObject(g_hdcBuffer, g_hbmBuffer);
  ReleaseDC(g_hWndCanvas, hdc);

  SyncBufferFromPixbuf();
}

static void CleanupCanvas(void) {
  if (g_hdcBuffer) {
    SelectObject(g_hdcBuffer, g_hbmOld);
    DeleteObject(g_hbmBuffer);
    DeleteDC(g_hdcBuffer);
    g_hdcBuffer = NULL;
  }

  if (g_canvas) {
    pixbuf_destroy(g_canvas);
    g_canvas = NULL;
  }

  if (g_temp_canvas) {
    pixbuf_destroy(g_temp_canvas);
    g_temp_canvas = NULL;
  }

  if (g_undo) {
    undo_destroy(g_undo);
    g_undo = NULL;
  }
}

static void SyncBufferFromPixbuf(void) {
  BITMAPINFO bmi;
  int y, x;
  unsigned char *row_buf;
  const unsigned char *src_row;

  if (!g_canvas || !g_hdcBuffer)
    return;

  memset(&bmi, 0, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = g_canvas->width;
  bmi.bmiHeader.biHeight = -g_canvas->height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 24;
  bmi.bmiHeader.biCompression = BI_RGB;

  row_buf = (unsigned char *)malloc(g_canvas->width * 3 + 4);
  if (!row_buf)
    return;

  for (y = 0; y < g_canvas->height; y++) {
    src_row = g_canvas->data + y * g_canvas->row_stride;

    for (x = 0; x < g_canvas->width; x++) {
      row_buf[x * 3 + 0] = src_row[x * 3 + 2];
      row_buf[x * 3 + 1] = src_row[x * 3 + 1];
      row_buf[x * 3 + 2] = src_row[x * 3 + 0];
    }

    SetDIBitsToDevice(g_hdcBuffer, 0, y, g_canvas->width, 1, 0, 0, 0, 1,
                      row_buf, &bmi, DIB_RGB_COLORS);
  }

  free(row_buf);
}

static void RefreshCanvas(void) { InvalidateRect(g_hWndCanvas, NULL, FALSE); }

static void UpdateTitle(void) {
  char title[300];
  const char *filename;

  if (g_state.filename[0]) {
    filename = strrchr(g_state.filename, '\\');
    if (filename) {
      filename++;
    } else {
      filename = g_state.filename;
    }
  } else {
    filename = "Untitled";
  }

  wsprintf(title, "%s%s - Paint", g_state.modified ? "*" : "", filename);
  SetWindowText(g_hWndMain, title);
}

static void UpdateStatusBar(void) {
  char status[256];
  wsprintf(status, "Tool: %s | Size: %d | Pos: (%d, %d) | FG: %d | BG: %d",
           g_tool_names[g_state.current_tool], g_state.brush_size,
           g_state.current_point.x, g_state.current_point.y,
           g_state.foreground_color, g_state.background_color);
  SetWindowText(g_hWndStatus, status);
}

static void HighlightSelectedTool(void) {
  int i;
  for (i = 0; i < MAX_TOOLS; i++) {
    SendMessage(g_hToolButtons[i], BM_SETCHECK,
                (i == g_state.current_tool) ? BST_CHECKED : BST_UNCHECKED, 0);
    if (g_is_nt_351) InvalidateRect(g_hToolButtons[i], NULL, TRUE);
  }
}

static void HighlightSelectedSize(void) {
  int i;
  for (i = 0; i < 5; i++) {
    SendMessage(g_hSizeButtons[i], BM_SETCHECK,
                (i == g_selected_size_index) ? BST_CHECKED : BST_UNCHECKED, 0);
    if (g_is_nt_351) InvalidateRect(g_hSizeButtons[i], NULL, TRUE);
  }
}

static void RedrawAllColorButtons(void) {
  int i;
  for (i = 0; i < MAX_COLORS; i++) {
    InvalidateRect(g_hColorButtons[i], NULL, TRUE);
  }
  /* Also redraw FG/BG display area */
  {
    RECT rc;
    rc.left = 5;
    rc.top = CANVAS_HEIGHT + 10;
    rc.right = 90;
    rc.bottom = CANVAS_HEIGHT + 65;
    InvalidateRect(g_hWndMain, &rc, TRUE);
  }
}

static int ConfirmSave(void) {
  int result;

  if (!g_state.modified)
    return IDNO;

  result = MessageBox(g_hWndMain, "The image has been modified. Save changes?",
                      "Paint", MB_YESNOCANCEL | MB_ICONQUESTION);

  if (result == IDYES) {
    DoFileSave();
  }

  return result;
}

static void DoFileNew(void) {
  PaintColor white;

  if (ConfirmSave() == IDCANCEL)
    return;

  white.r = 255;
  white.g = 255;
  white.b = 255;
  pixbuf_clear(g_canvas, white);
  pixbuf_clear(g_temp_canvas, white);

  undo_clear(g_undo);

  g_state.filename[0] = '\0';
  g_state.modified = 0;

  SyncBufferFromPixbuf();
  RefreshCanvas();
  UpdateTitle();
}

static void DoFileOpen(void) {
  OPENFILENAME ofn;
  char filename[260];
  PixelBuffer *loaded;

  if (ConfirmSave() == IDCANCEL)
    return;

  memset(&ofn, 0, sizeof(ofn));
  memset(filename, 0, sizeof(filename));

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = g_hWndMain;
  ofn.lpstrFilter = "Bitmap Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = filename;
  ofn.nMaxFile = sizeof(filename);
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = "bmp";

  if (!GetOpenFileName(&ofn))
    return;

  loaded = bmp_load(filename);
  if (!loaded) {
    MessageBox(g_hWndMain,
               "Failed to open file.\nMake sure it's a valid 24-bit BMP.",
               "Error", MB_ICONERROR | MB_OK);
    return;
  }

  pixbuf_destroy(g_canvas);
  g_canvas = loaded;

  pixbuf_destroy(g_temp_canvas);
  g_temp_canvas = pixbuf_create(g_canvas->width, g_canvas->height);
  memcpy(g_temp_canvas->data, g_canvas->data,
         g_canvas->row_stride * g_canvas->height);

  undo_clear(g_undo);

  lstrcpy(g_state.filename, filename);
  g_state.modified = 0;

  SyncBufferFromPixbuf();
  RefreshCanvas();
  UpdateTitle();
}

static void DoFileSave(void) {
  if (g_state.filename[0] == '\0') {
    DoFileSaveAs();
    return;
  }

  if (!bmp_save(g_state.filename, g_canvas)) {
    MessageBox(g_hWndMain, "Failed to save file.", "Error",
               MB_ICONERROR | MB_OK);
    return;
  }

  g_state.modified = 0;
  UpdateTitle();
}

static void DoFileSaveAs(void) {
  OPENFILENAME ofn;
  char filename[260];

  memset(&ofn, 0, sizeof(ofn));
  memset(filename, 0, sizeof(filename));

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = g_hWndMain;
  ofn.lpstrFilter = "Bitmap Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = filename;
  ofn.nMaxFile = sizeof(filename);
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = "bmp";

  if (!GetSaveFileName(&ofn))
    return;

  lstrcpy(g_state.filename, filename);
  DoFileSave();
}

static void DoUndo(void) {
  PixelBuffer *prev;

  if (!undo_can_undo(g_undo)) {
    return;
  }

  prev = undo_pop(g_undo);
  if (prev) {
    pixbuf_destroy(g_canvas);
    g_canvas = prev;

    memcpy(g_temp_canvas->data, g_canvas->data,
           g_canvas->row_stride * g_canvas->height);

    g_state.modified = 1;
    SyncBufferFromPixbuf();
    RefreshCanvas();
    UpdateTitle();
  }
}

static void DoClear(void) {
  PaintColor bg;

  undo_push(g_undo, g_canvas);

  bg = g_palette[g_state.background_color];
  pixbuf_clear(g_canvas, bg);
  memcpy(g_temp_canvas->data, g_canvas->data,
         g_canvas->row_stride * g_canvas->height);

  g_state.modified = 1;
  SyncBufferFromPixbuf();
  RefreshCanvas();
  UpdateTitle();
}

/* Drawing functions */
static void OnMouseDown(int x, int y, int button) {
  PaintColor fg;

  g_state.is_drawing = 1;
  g_state.drawing_button = button;
  g_state.start_point.x = x;
  g_state.start_point.y = y;
  g_state.last_point.x = x;
  g_state.last_point.y = y;
  g_state.current_point.x = x;
  g_state.current_point.y = y;

  SetCapture(g_hWndCanvas);

  /* Save for undo before drawing starts */
  undo_push(g_undo, g_canvas);

  /* Copy canvas to temp for rubber-banding */
  memcpy(g_temp_canvas->data, g_canvas->data,
         g_canvas->row_stride * g_canvas->height);

  if (g_state.current_tool == TOOL_FILL) {
    fg = (button == 2) ? g_palette[g_state.background_color]
                       : g_palette[g_state.foreground_color];
    flood_fill_area(g_canvas, x, y, fg);
    memcpy(g_temp_canvas->data, g_canvas->data,
           g_canvas->row_stride * g_canvas->height);
    g_state.is_drawing = 0;
    g_state.modified = 1;
    ReleaseCapture();
    SyncBufferFromPixbuf();
    RefreshCanvas();
    UpdateTitle();
  } else if (g_state.current_tool == TOOL_PICKER) {
    PaintColor picked = pixbuf_get_pixel(g_canvas, x, y);
    int i, best = 0;
    int best_dist = 0x7FFFFFFF;

    for (i = 0; i < MAX_COLORS; i++) {
      int dr = (int)picked.r - (int)g_palette[i].r;
      int dg = (int)picked.g - (int)g_palette[i].g;
      int db = (int)picked.b - (int)g_palette[i].b;
      int dist = dr * dr + dg * dg + db * db;
      if (dist < best_dist) {
        best_dist = dist;
        best = i;
      }
    }

    if (button == 1) {
      g_state.foreground_color = best;
    } else {
      g_state.background_color = best;
    }

    g_state.is_drawing = 0;
    ReleaseCapture();
    RedrawAllColorButtons();
    UpdateStatusBar();

    /* Pop the undo since we didn't draw anything */
    undo_pop(g_undo);
  } else if (g_state.current_tool == TOOL_PENCIL ||
             g_state.current_tool == TOOL_BRUSH ||
             g_state.current_tool == TOOL_AIRBRUSH ||
             g_state.current_tool == TOOL_ERASER) {
    DrawCurrentTool(x, y, x, y, 0);
    SyncBufferFromPixbuf();
    RefreshCanvas();
  }

  UpdateStatusBar();
}

static void OnMouseMove(int x, int y) {
  x = clamp_int(x, 0, g_canvas->width - 1);
  y = clamp_int(y, 0, g_canvas->height - 1);

  g_state.current_point.x = x;
  g_state.current_point.y = y;

  if (g_state.is_drawing) {
    switch (g_state.current_tool) {
    case TOOL_PENCIL:
    case TOOL_BRUSH:
    case TOOL_ERASER:
      DrawCurrentTool(g_state.last_point.x, g_state.last_point.y, x, y, 0);
      SyncBufferFromPixbuf();
      RefreshCanvas();
      break;

    case TOOL_AIRBRUSH:
      DrawCurrentTool(x, y, x, y, 0);
      SyncBufferFromPixbuf();
      RefreshCanvas();
      break;

    case TOOL_LINE:
    case TOOL_RECT:
    case TOOL_FILLRECT:
    case TOOL_ELLIPSE:
    case TOOL_FILLELLIPSE:
    case TOOL_ROUNDRECT:
    case TOOL_FILLROUNDRECT:
      memcpy(g_temp_canvas->data, g_canvas->data,
             g_canvas->row_stride * g_canvas->height);
      DrawCurrentTool(g_state.start_point.x, g_state.start_point.y, x, y, 1);
      {
        PixelBuffer *swap = g_canvas;
        g_canvas = g_temp_canvas;
        SyncBufferFromPixbuf();
        g_canvas = swap;
      }
      RefreshCanvas();
      break;
    }

    g_state.last_point.x = x;
    g_state.last_point.y = y;
  }

  UpdateStatusBar();
}

static void OnMouseUp(int x, int y, int button) {
  if (!g_state.is_drawing)
    return;

  x = clamp_int(x, 0, g_canvas->width - 1);
  y = clamp_int(y, 0, g_canvas->height - 1);

  switch (g_state.current_tool) {
  case TOOL_LINE:
  case TOOL_RECT:
  case TOOL_FILLRECT:
  case TOOL_ELLIPSE:
  case TOOL_FILLELLIPSE:
  case TOOL_ROUNDRECT:
  case TOOL_FILLROUNDRECT:
    DrawCurrentTool(g_state.start_point.x, g_state.start_point.y, x, y, 0);
    break;
  }

  memcpy(g_temp_canvas->data, g_canvas->data,
         g_canvas->row_stride * g_canvas->height);

  g_state.is_drawing = 0;
  g_state.modified = 1;
  ReleaseCapture();

  SyncBufferFromPixbuf();
  RefreshCanvas();
  UpdateTitle();
}

static void DrawCurrentTool(int x1, int y1, int x2, int y2, int to_temp) {
  PixelBuffer *target = to_temp ? g_temp_canvas : g_canvas;
  PaintColor fg = (g_state.drawing_button == 2)
                      ? g_palette[g_state.background_color]
                      : g_palette[g_state.foreground_color];
  PaintColor bg = g_palette[g_state.background_color];
  int size = g_state.brush_size;
  int radius;

  switch (g_state.current_tool) {
  case TOOL_PENCIL:
    draw_line(target, x1, y1, x2, y2, fg, 1);
    break;

  case TOOL_BRUSH:
    draw_line(target, x1, y1, x2, y2, fg, size);
    break;

  case TOOL_AIRBRUSH:
    draw_airbrush(target, x2, y2, fg, size * 2, g_state.airbrush_density);
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
    if (radius < 5)
      radius = 5;
    draw_round_rect(target, x1, y1, x2, y2, fg, size, 0, radius);
    break;

  case TOOL_FILLROUNDRECT:
    radius = min_int(abs_int(x2 - x1), abs_int(y2 - y1)) / 4;
    if (radius < 5)
      radius = 5;
    draw_round_rect(target, x1, y1, x2, y2, fg, size, 1, radius);
    break;

  case TOOL_ERASER:
    draw_line(target, x1, y1, x2, y2, bg, size * 2);
    break;
  }
}

static void DrawColorButton(HDC hdc, int index, RECT *pRect, int is_fg,
                            int is_bg) {
  HBRUSH hBrush;
  HPEN hPen, hOldPen;
  RECT rc = *pRect;

  /* Fill with the color */
  hBrush = CreateSolidBrush(g_colors[index]);
  FillRect(hdc, &rc, hBrush);
  DeleteObject(hBrush);

  /* Draw selection indicators */
  if (is_fg) {
    /* Foreground: white border */
    hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    hOldPen = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, rc.left + 1, rc.top + 1, NULL);
    LineTo(hdc, rc.right - 2, rc.top + 1);
    LineTo(hdc, rc.right - 2, rc.bottom - 2);
    LineTo(hdc, rc.left + 1, rc.bottom - 2);
    LineTo(hdc, rc.left + 1, rc.top + 1);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
  }

  if (is_bg) {
    /* Background: black dashed inner border */
    hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    hOldPen = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, rc.left + 3, rc.top + 3, NULL);
    LineTo(hdc, rc.right - 4, rc.top + 3);
    LineTo(hdc, rc.right - 4, rc.bottom - 4);
    LineTo(hdc, rc.left + 3, rc.bottom - 4);
    LineTo(hdc, rc.left + 3, rc.top + 3);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
  }

  /* Outer black border */
  FrameRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
}

static void DrawFgBgDisplay(HWND hWnd, HDC hdc) {
  RECT rcFg, rcBg;
  HBRUSH hBrush;
  HFONT hFont, hOldFont;

  /* Background color box (behind, offset right and down) */
  rcBg.left = 25;
  rcBg.top = CANVAS_HEIGHT + 32;
  rcBg.right = rcBg.left + 28;
  rcBg.bottom = rcBg.top + 28;

  hBrush = CreateSolidBrush(g_colors[g_state.background_color]);
  FillRect(hdc, &rcBg, hBrush);
  DeleteObject(hBrush);
  FrameRect(hdc, &rcBg, (HBRUSH)GetStockObject(BLACK_BRUSH));

  /* Foreground color box (in front, offset left and up) */
  rcFg.left = 10;
  rcFg.top = CANVAS_HEIGHT + 22;
  rcFg.right = rcFg.left + 28;
  rcFg.bottom = rcFg.top + 28;

  hBrush = CreateSolidBrush(g_colors[g_state.foreground_color]);
  FillRect(hdc, &rcFg, hBrush);
  DeleteObject(hBrush);
  FrameRect(hdc, &rcFg, (HBRUSH)GetStockObject(BLACK_BRUSH));

  /* Label */
  hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  hOldFont = (HFONT)SelectObject(hdc, hFont);
  SetBkMode(hdc, TRANSPARENT);
  TextOut(hdc, 10, CANVAS_HEIGHT + 10, "FG/BG:", 6);
  SelectObject(hdc, hOldFont);
}

/* Main window procedure */
static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam) {
  int id;
  int i;
  PAINTSTRUCT ps;
  HDC hdc;
  DRAWITEMSTRUCT *pdis;

  switch (msg) {
  case WM_CREATE:
    CreateMainMenu(hWnd);
    CreateToolPanel(hWnd);
    CreateCanvasWindow(hWnd);
    CreateColorPanel(hWnd);
    CreateStatusBar(hWnd);
    return 0;

  case WM_PAINT:
    hdc = BeginPaint(hWnd, &ps);
    DrawFgBgDisplay(hWnd, hdc);
    EndPaint(hWnd, &ps);
    return 0;

  case WM_COMMAND:
    id = LOWORD(wParam);

    /* Tool buttons */
    if (id >= IDB_TOOL_BASE && id < IDB_TOOL_BASE + MAX_TOOLS) {
      g_state.current_tool = id - IDB_TOOL_BASE;
      HighlightSelectedTool();
      UpdateStatusBar();
      return 0;
    }

    /* Size buttons */
    if (id >= IDB_SIZE_BASE && id < IDB_SIZE_BASE + 5) {
      g_selected_size_index = id - IDB_SIZE_BASE;
      g_state.brush_size = g_brush_sizes[g_selected_size_index];
      HighlightSelectedSize();
      UpdateStatusBar();
      return 0;
    }

    /* Color buttons - left click only through WM_COMMAND */
    if (id >= IDB_COLOR_BASE && id < IDB_COLOR_BASE + MAX_COLORS) {
      g_state.foreground_color = id - IDB_COLOR_BASE;
      RedrawAllColorButtons();
      UpdateStatusBar();
      return 0;
    }

    /* Menu commands */
    switch (id) {
    case IDM_FILE_NEW:
      DoFileNew();
      return 0;

    case IDM_FILE_OPEN:
      DoFileOpen();
      return 0;

    case IDM_FILE_SAVE:
      DoFileSave();
      return 0;

    case IDM_FILE_SAVEAS:
      DoFileSaveAs();
      return 0;

    case IDM_FILE_EXIT:
      if (ConfirmSave() != IDCANCEL) {
        PostQuitMessage(0);
      }
      return 0;

    case IDM_EDIT_UNDO:
      DoUndo();
      return 0;

    case IDM_EDIT_CLEAR:
      DoClear();
      return 0;

    case IDM_HELP_ABOUT:
      MessageBox(hWnd,
                 "Portable Paint Program v2.0\n\n"
                 "A full-featured MS Paint-like application\n"
                 "Compatible with Win32 and X11/Motif\n\n"
                 "Tools: Pencil, Brush, Airbrush, Line,\n"
                 "Rectangle, Ellipse, Rounded Rect,\n"
                 "Eraser, Fill, Color Picker\n\n"
                 "Left-click: Draw with foreground color\n"
                 "Right-click: Draw with background color\n"
                 "Right-click on color: Set background color",
                 "About Paint", MB_OK | MB_ICONINFORMATION);
      return 0;
    }
    break;

  case WM_DRAWITEM:
    pdis = (DRAWITEMSTRUCT *)lParam;
    id = (int)wParam;

    /* Color buttons */
    if (id >= IDB_COLOR_BASE && id < IDB_COLOR_BASE + MAX_COLORS) {
      int color_idx = id - IDB_COLOR_BASE;
      int is_fg = (color_idx == g_state.foreground_color);
      int is_bg = (color_idx == g_state.background_color);

      DrawColorButton(pdis->hDC, color_idx, &pdis->rcItem, is_fg, is_bg);
      return TRUE;
    }

    /* Tool buttons (legacy support) */
    if (g_is_nt_351 && id >= IDB_TOOL_BASE && id < IDB_TOOL_BASE + MAX_TOOLS) {
        int tool_idx = id - IDB_TOOL_BASE;
        char text[32];
        GetWindowText(pdis->hwndItem, text, sizeof(text));
        Draw3DButton(pdis->hDC, &pdis->rcItem, text, (tool_idx == g_state.current_tool));
        return TRUE;
    }

    /* Size buttons (legacy support) */
    if (g_is_nt_351 && id >= IDB_SIZE_BASE && id < IDB_SIZE_BASE + 5) {
        int size_idx = id - IDB_SIZE_BASE;
        char text[16];
        GetWindowText(pdis->hwndItem, text, sizeof(text));
        Draw3DButton(pdis->hDC, &pdis->rcItem, text, (size_idx == g_selected_size_index));
        return TRUE;
    }
    break;

  case WM_CONTEXTMENU:
    /* Handle right-click on color buttons */
    {
      POINT pt;
      RECT btnRect;

      pt.x = LOWORD(lParam);
      pt.y = HIWORD(lParam);

      for (i = 0; i < MAX_COLORS; i++) {
        GetWindowRect(g_hColorButtons[i], &btnRect);
        if (PtInRect(&btnRect, pt)) {
          g_state.background_color = i;
          RedrawAllColorButtons();
          UpdateStatusBar();
          return 0;
        }
      }
    }
    break;

  case WM_CLOSE:
    if (ConfirmSave() == IDCANCEL)
      return 0;
    DestroyWindow(hWnd);
    return 0;

  case WM_DESTROY:
    CleanupCanvas();
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}

/* Canvas window procedure */
static LRESULT CALLBACK CanvasWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam) {
  HDC hdc;
  PAINTSTRUCT ps;
  int x, y;

  switch (msg) {
  case WM_CREATE:
    InitializeCanvas();
    return 0;

  case WM_PAINT:
    hdc = BeginPaint(hWnd, &ps);
    if (g_hdcBuffer) {
      BitBlt(hdc, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, g_hdcBuffer, 0, 0,
             SRCCOPY);
    }
    EndPaint(hWnd, &ps);
    return 0;

  case WM_LBUTTONDOWN:
    x = (short)LOWORD(lParam);
    y = (short)HIWORD(lParam);
    OnMouseDown(x, y, 1);
    return 0;

  case WM_RBUTTONDOWN:
    x = (short)LOWORD(lParam);
    y = (short)HIWORD(lParam);
    OnMouseDown(x, y, 2);
    return 0;

  case WM_MOUSEMOVE:
    x = (short)LOWORD(lParam);
    y = (short)HIWORD(lParam);
    OnMouseMove(x, y);
    return 0;

  case WM_LBUTTONUP:
    x = (short)LOWORD(lParam);
    y = (short)HIWORD(lParam);
    OnMouseUp(x, y, 1);
    return 0;

  case WM_RBUTTONUP:
    x = (short)LOWORD(lParam);
    y = (short)HIWORD(lParam);
    OnMouseUp(x, y, 2);
    return 0;

  case WM_SETCURSOR:
    SetCursor(LoadCursor(NULL, IDC_CROSS));
    return TRUE;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}