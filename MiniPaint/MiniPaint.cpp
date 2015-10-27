// MiniPaint.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "MiniPaint.h"
#include "Canvas.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
int controlMargin = 10;							// distance between child windows and window borders
HWND g_hWnd;									// handle to the main window
HMENU hMenu;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL				CreateCanvas();
void				ExclusiveCheckMenuItem(UINT uIDCheckItem);
BOOL				ShowColorDialog(COLORREF* color);
void				OnSize();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MINIPAINT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MINIPAINT));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MINIPAINT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MINIPAINT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   g_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!g_hWnd)
      return FALSE;

   hMenu = GetMenu(g_hWnd);

   if (!CreateCanvas())
	   return FALSE;

   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case ID_TOOLS_REFRESH:
				Refresh();
				break;
			case ID_TOOLS_CLEAR:
				Clear();
				break;
			case ID_TOOLS_PENCIL:
				ExclusiveCheckMenuItem(ID_TOOLS_PENCIL);
				SetPaintTool(PaintTool::Pencil);
				break;
			case ID_TOOLS_LINE:
				ExclusiveCheckMenuItem(ID_TOOLS_LINE);
				SetPaintTool(PaintTool::Line);
				break;
			case ID_TOOLS_POLYLINE:
				ExclusiveCheckMenuItem(ID_TOOLS_POLYLINE);
				SetPaintTool(PaintTool::Polyline);
				break;
			case ID_TOOLS_RECTANGLE:
				ExclusiveCheckMenuItem(ID_TOOLS_RECTANGLE);
				SetPaintTool(PaintTool::Rectangle);
				break;
			case ID_TOOLS_PENCOLOR:
			{
				COLORREF color = GetPenColor();
				if (ShowColorDialog(&color))
					SetPenColor(color);
			}
				break;
			case ID_TOOLS_BRUSHCOLOR:
			{
				COLORREF color = GetBrushColor();
				if (ShowColorDialog(&color))
					SetBrushColor(color);
			}
				break;
			case ID_FILE_SAVE:
			{
				wchar_t filename[300];
				OPENFILENAME fileInfo;
				ZeroMemory(&fileInfo, sizeof(OPENFILENAME));
				fileInfo.lStructSize = sizeof(OPENFILENAME);
				fileInfo.hwndOwner = g_hWnd;
				fileInfo.hInstance = hInst;
				fileInfo.lpstrFilter = L"Enhanced metafile\0*.emf\0";
				fileInfo.nFilterIndex = 1;
				fileInfo.lpstrFile = filename;
				fileInfo.lpstrFile[0] = '\0';
				fileInfo.nMaxFile = 300;
				fileInfo.lpstrDefExt = L".emf";

				GetSaveFileName(&fileInfo);

				SaveToMetafile(fileInfo.lpstrFile);
			}
				break;
			case ID_FILE_OPEN:
			{
				wchar_t filename[300];
				OPENFILENAME fileInfo;
				ZeroMemory(&fileInfo, sizeof(OPENFILENAME));
				fileInfo.lStructSize = sizeof(OPENFILENAME);
				fileInfo.hwndOwner = g_hWnd;
				fileInfo.hInstance = hInst;
				fileInfo.lpstrFilter = L"Enhanced metafile\0*.emf\0";
				fileInfo.nFilterIndex = 1;
				fileInfo.lpstrFile = filename;
				fileInfo.lpstrFile[0] = '\0';
				fileInfo.nMaxFile = 300;
				fileInfo.Flags = OFN_FILEMUSTEXIST;

				GetOpenFileName(&fileInfo);

				LoadFromMetafile(fileInfo.lpstrFile);
			}
				break;
			case ID_TOOLS_INCREASEPENTHICKNESS:
				IncreasePenThickness();
				break;
			case ID_TOOLS_DECREASEPENTHICKNESS:
				DecreasePenThickness();
				break;
			case ID_EDIT_UNDO:
				Undo();
				break;
			case ID_EDIT_REDO:
				Redo();
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
				break;
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_MOUSEWHEEL:
		OnMouseWheel(wParam);
		break;
	case WM_SIZE:
		OnSize();
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//Calculates the size and position of the canvas window and creates it
BOOL CreateCanvas()
{
	RECT clientArea;
	if (!GetClientRect(g_hWnd, &clientArea))
		return FALSE;

	int x, y, height, width;
	x = controlMargin;
	y = controlMargin;
	height = clientArea.bottom - clientArea.top - 2 * controlMargin;
	width = clientArea.right - clientArea.left - 2 * controlMargin;

	return CreateCanvasWindow(hInst, g_hWnd, x, y, width, height);
}

//Checks a menu item and unchecks all the rest.
void ExclusiveCheckMenuItem(UINT uIDCheckItem)
{
	CheckMenuItem(hMenu, ID_TOOLS_PENCIL, MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TOOLS_LINE, MF_UNCHECKED);

	CheckMenuItem(hMenu, uIDCheckItem, MF_CHECKED);
}

//Initializes and shows a color dialog box, assignes the result to color if the dialog is successful.
BOOL ShowColorDialog(COLORREF* color)
{
	COLORREF a[16];
	CHOOSECOLOR c;
	ZeroMemory(&c, sizeof(c));
	c.lStructSize = sizeof(c);
	c.hwndOwner = g_hWnd;
	c.rgbResult = *color;
	c.Flags = CC_RGBINIT;
	c.lpCustColors = (LPDWORD)a;

	if (ChooseColor(&c))
	{
		*color = c.rgbResult;
		return TRUE;
	}
	
	return FALSE;
}

//Handles the window resize message (WM_SIZE).
void OnSize()
{
	RECT clientRect;
	GetClientRect(g_hWnd, &clientRect);

	int x = 0, y = 0, height = 0, width = 0;
	x = controlMargin;
	y = controlMargin;
	width = clientRect.right - clientRect.left - 2 * controlMargin;
	height = clientRect.bottom - clientRect.top - 2 * controlMargin;

	ResizeWindow(x, y, width, height);
}