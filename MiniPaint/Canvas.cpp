#include "stdafx.h"
#include "Canvas.h"
#include <list>
#include "Line.h"
#include "PolygonalChain.h"
#include <ctgmath>


HINSTANCE hInstance;
HWND parentWindow, thisWindow;
LPCWSTR thisClassName = L"canvas";
LPCWSTR thisWindowName = L"Canvas";
bool isWindowCreated;

int canvasWidth = 2000;
int canvasHeight = 2000;
HDC bufferDC;

int verticalScrollPosition = 0;
int verticalScrollMax;
int verticalScrollLineSize = 20;
int horizontalScrollPosition = 0;
int horizontalScrollMax;
int horizontalScrollLineSize = 20;
double zoomFactor = 1;
double zoomDelta = 0.2;
int invalidationExcess = 3;

bool isDragging;

PaintTool currentTool = PaintTool::None;
std::list<PaintAction*> actions;
std::list<PaintAction*> undoneActions;

COLORREF penColor = RGB(0, 0, 0);
HPEN currentPen;


ATOM				RegisterCanvasAreaClass();
LRESULT CALLBACK	CanvasWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void				OnLButtonDown();
void				OnLButtonUp();
void				OnMouseMove(LPARAM lParam);
void				OnPaint();
void				OnVerticalScroll(WPARAM wParam);
void				OnHorizontalScroll(WPARAM wParam);
void				ConfigureScrollBars();
void				ScrollLineUp();
void				ScrollLineDown();
void				ScrollLineLeft();
void				ScrollLineRight();
void				RequestRepaintWindow();
void				ClearBuffer();
void				InitBufferDC();
void				RefreshBuffer();
void				ZoomIn();
void				ZoomOut();
void				OnZoomFactorChanged(double oldZoomFactor);
void				GetAdjustedCursorPos(LPPOINT point);
void				AdjustedToClientPoint(LPPOINT point);
BOOL CreateCanvasWindow(HINSTANCE hInst, HWND parentHWnd, int x, int y, int width, int height)
{
	if (isWindowCreated)
		return FALSE;

	hInstance = hInst;
	parentWindow = parentHWnd;

	if (!RegisterCanvasAreaClass())
		return FALSE;

	thisWindow = CreateWindow(
		thisClassName, 
		thisWindowName,
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL,
		x, 
		y, 
		width, 
		height, 
		parentHWnd, 
		nullptr, 
		hInst, 
		nullptr);

	if (!thisWindow)
		return FALSE;

	isWindowCreated = true;
	UpdateWindow(thisWindow);

	InitBufferDC();
	ConfigureScrollBars();

	return TRUE;
}

ATOM RegisterCanvasAreaClass()
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = CanvasWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = thisClassName;
	wcex.hIconSm = nullptr;

	return RegisterClassExW(&wcex);
}

LRESULT CALLBACK CanvasWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_VSCROLL:
		OnVerticalScroll(wParam);
		break;
	case WM_HSCROLL:
		OnHorizontalScroll(wParam);
		break;
	case WM_PAINT:
		OnPaint();
		break;
	case WM_LBUTTONDOWN:
	{
		OnLButtonDown();
	}
	break;
	case WM_MOUSELEAVE:
	case WM_LBUTTONUP:
	{
		OnLButtonUp();
	}
	break;
	case WM_MOUSEMOVE:
	{
		OnMouseMove(lParam);
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void OnMouseWheel(WPARAM wParam)
{
	short int rotationDistance = (short int)HIWORD(wParam);
	int downKeys = LOWORD(wParam);

	if (downKeys & MK_CONTROL)		//Ctrl(+Shift)+Wheel - zoom
	{
		rotationDistance > 0 ? ZoomIn() : ZoomOut();
	}
	else							//scroll
	{
		if (downKeys & MK_SHIFT)
			rotationDistance > 0 ? ScrollLineLeft() : ScrollLineRight();
		else
			rotationDistance > 0 ? ScrollLineUp() : ScrollLineDown();
	}

	ConfigureScrollBars();
	RequestRepaintWindow();
}

void OnLButtonDown()
{
	isDragging = true;

	TRACKMOUSEEVENT trmEvent;
	trmEvent.cbSize = sizeof(TRACKMOUSEEVENT);
	trmEvent.dwFlags = TME_LEAVE;
	trmEvent.hwndTrack = thisWindow;
	TrackMouseEvent(&trmEvent);

	POINT mousePos;
	GetAdjustedCursorPos(&mousePos);

	switch (currentTool)
	{
	case PaintTool::Pencil:
	{
		MoveToEx(bufferDC, mousePos.x, mousePos.y, nullptr);

		PolygonalChain* pl = new PolygonalChain();
		pl->AddVertex(mousePos.x, mousePos.y);
		actions.push_back(pl);
	}
	break;
	case PaintTool::Line:
	{
		Line* line = new Line(mousePos.x, mousePos.y);
		actions.push_back(line);
	}
	break;
	default:
		break;
	}
}

void OnLButtonUp()
{
	isDragging = false;

	switch (currentTool)
	{
	case PaintTool::Line:
	{
		actions.back()->Draw(bufferDC, 0, 0);
	}
	break;
	default:
		break;
	}
}

void OnMouseMove(LPARAM lParam)
{
	if (isDragging)
	{
		POINT mousePos;
		GetAdjustedCursorPos(&mousePos);

		//area to be invalidated
		RECT invRect;

		switch (currentTool)
		{
		case PaintTool::Pencil:
		{
			PolygonalChain* pl = (PolygonalChain*)actions.back();
			POINT prevPoint;
			pl->GetLastVertex(&prevPoint);
			pl->AddVertex(mousePos.x, mousePos.y);

			LineTo(bufferDC, mousePos.x, mousePos.y);

			AdjustedToClientPoint(&prevPoint);
			AdjustedToClientPoint(&mousePos);
			invRect.bottom = max(prevPoint.y, mousePos.y) + invalidationExcess;
			invRect.top = min(prevPoint.y, mousePos.y) - invalidationExcess;
			invRect.left = min(prevPoint.x, mousePos.x) - invalidationExcess;
			invRect.right = max(prevPoint.x, mousePos.x) + invalidationExcess;
		}
		break;
		case PaintTool::Line:
		{
			Line* line = (Line*)actions.back();

			POINT P1, prevP2;
			line->GetP1(&P1);
			line->GetP2(&prevP2);
			
			line->x2 = mousePos.x;
			line->y2 = mousePos.y;
			
			AdjustedToClientPoint(&P1);
			AdjustedToClientPoint(&prevP2);
			AdjustedToClientPoint(&mousePos);
			invRect.bottom = max(max(P1.y, prevP2.y), mousePos.y) + invalidationExcess;
			invRect.top = min(min(P1.y, prevP2.y), mousePos.y) - invalidationExcess;
			invRect.left = min(min(P1.x, prevP2.x), mousePos.x) - invalidationExcess;
			invRect.right = max(max(P1.x, prevP2.x), mousePos.x) + invalidationExcess;
		}
		break;
		default:
			break;
		}

		InvalidateRect(thisWindow, &invRect, TRUE);
		UpdateWindow(thisWindow);
	}
}

//Handles the paint message (WM_PAINT).
void OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(thisWindow, &ps);
	SelectObject(hdc, currentPen);

	RECT clientArea;
	GetClientRect(thisWindow, &clientArea);
	int destWidth = clientArea.right - clientArea.left;
	int destHeight = clientArea.bottom - clientArea.top;
	int srcWidth = (int)(destWidth / zoomFactor);
	int srcHeight = (int)(destHeight / zoomFactor);
	int srcX = (int)(horizontalScrollPosition / zoomFactor);
	int srcY = (int)(verticalScrollPosition / zoomFactor);

	BOOL res = StretchBlt(hdc, 0, 0, destWidth, destHeight, bufferDC, srcX, srcY, srcWidth, srcHeight, SRCCOPY);

	if (isDragging)
	{
		switch (currentTool)
		{
		case PaintTool::Line:
		{
			Line* line = (Line*)actions.back();
			
			MoveToEx(hdc, round(line->x1 * zoomFactor), round(line->y1 * zoomFactor), nullptr);
			LineTo(hdc, round(line->x2 * zoomFactor), round(line->y2 * zoomFactor));
		}
		break;
		default:
			break;
		}
	}

	EndPaint(thisWindow, &ps);
}

//Hadles the vertical scroll message (WM_VSCROLL).
void OnVerticalScroll(WPARAM wParam)
{
	WORD scrReq = LOWORD(wParam);

	switch (scrReq)
	{
	case SB_LINEDOWN:
	{
		ScrollLineDown();
	}
	break;
	case SB_LINEUP:
	{
		ScrollLineUp();
	}
	break;
	case SB_THUMBTRACK:
	{
		WORD position = HIWORD(wParam);
		ScrollWindowEx(thisWindow, 0, verticalScrollPosition - position, nullptr, nullptr, nullptr, nullptr, 0);
		verticalScrollPosition = position;
	}
	break;
	default:
		break;
	}

	ConfigureScrollBars();
	RequestRepaintWindow();
}

//Hadles the horizontal scroll message (WM_HSCROLL).
void OnHorizontalScroll(WPARAM wParam)
{
	WORD scrReq = LOWORD(wParam);

	switch (scrReq)
	{
	case SB_LINELEFT:
	{
		ScrollLineLeft();
	}
	break;
	case SB_LINERIGHT:
	{
		ScrollLineRight();
	}
	break;
	case SB_THUMBTRACK:
	{
		WORD position = HIWORD(wParam);
		ScrollWindowEx(thisWindow, horizontalScrollPosition - position, 0, nullptr, nullptr, nullptr, nullptr, 0);
		horizontalScrollPosition = position;
	}
	break;
	default:
		break;
	}

	ConfigureScrollBars();
	RequestRepaintWindow();
}

//Sets up the scrollbars.
void ConfigureScrollBars()
{
	RECT clientRect;
	GetClientRect(thisWindow, &clientRect);

	SCROLLINFO siVert;
	siVert.cbSize = sizeof(SCROLLINFO);
	siVert.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;
	siVert.nPos = verticalScrollPosition;         // scrollbar thumb position
	siVert.nPage = clientRect.bottom - clientRect.top;        // number of lines in a page (i.e. rows of text in window)
	siVert.nMin = 0;
	siVert.nMax = round(canvasHeight * zoomFactor);
	SetScrollInfo(thisWindow, SB_VERT, &siVert, TRUE);

	SCROLLINFO siHorz;
	siHorz.cbSize = sizeof(SCROLLINFO);
	siHorz.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;
	siHorz.nPos = horizontalScrollPosition;         // scrollbar thumb position
	siHorz.nPage = clientRect.right - clientRect.left;        // number of lines in a page (i.e. rows of text in window)
	siHorz.nMin = 0;
	siHorz.nMax = round(canvasWidth * zoomFactor);
	SetScrollInfo(thisWindow, SB_HORZ, &siHorz, TRUE);
}

//Scrolls the window one line up. Does not repaint.
void ScrollLineUp()
{
	if (verticalScrollPosition - verticalScrollLineSize >= verticalScrollLineSize)
	{
		verticalScrollPosition -= verticalScrollLineSize;
		ScrollWindowEx(thisWindow, 0, verticalScrollLineSize, nullptr, nullptr, nullptr, nullptr, 0);
	}
	else
	{
		if (verticalScrollPosition > 0)	//not at the top, but less than 1 line from the top
		{
			int finalVertScrollDelta = verticalScrollPosition;
			verticalScrollPosition -= finalVertScrollDelta;
			ScrollWindowEx(thisWindow, 0, finalVertScrollDelta, nullptr, nullptr, nullptr, nullptr, 0);
		}
	}
}

//Scrolls the window one line down. Does not repaint.
void ScrollLineDown()
{
	RECT clientArea;
	if (!GetClientRect(thisWindow, &clientArea))
		return;
	int windowHeight = clientArea.bottom - clientArea.top;

	if (verticalScrollPosition + verticalScrollLineSize + windowHeight <= canvasHeight)
	{
		verticalScrollPosition += verticalScrollLineSize;
		ScrollWindowEx(thisWindow, 0, -verticalScrollLineSize, nullptr, nullptr, nullptr, nullptr, 0);
	}
	else
	{
		if (verticalScrollPosition + windowHeight < canvasHeight)	//not at the bottom, but less than 1 line to the bottom
		{
			int finalVertScrollDelta = canvasHeight - (verticalScrollPosition + windowHeight);
			verticalScrollPosition += finalVertScrollDelta;
			ScrollWindowEx(thisWindow, 0, -finalVertScrollDelta, nullptr, nullptr, nullptr, nullptr, 0);
		}
	}
}

//Scrolls the window one line left. Does not repaint.
void ScrollLineLeft()
{
	if (horizontalScrollPosition - horizontalScrollLineSize >= horizontalScrollLineSize)
	{
		horizontalScrollPosition -= horizontalScrollLineSize;
		ScrollWindowEx(thisWindow, horizontalScrollLineSize, 0, nullptr, nullptr, nullptr, nullptr, 0);
	}
	else
	{
		if (horizontalScrollPosition > 0)	//not at the extreme left, but less than 1 line from there
		{
			int finalVertScrollDelta = horizontalScrollPosition;
			horizontalScrollPosition -= finalVertScrollDelta;
			ScrollWindowEx(thisWindow, finalVertScrollDelta, 0, nullptr, nullptr, nullptr, nullptr, 0);
		}
	}
}

//Scrolls the window one line right. Does not repaint.
void ScrollLineRight()
{
	RECT clientArea;
	if (!GetClientRect(thisWindow, &clientArea))
		return;
	int windowWidth = clientArea.right - clientArea.left;

	if (horizontalScrollPosition + horizontalScrollLineSize + windowWidth <= canvasHeight)
	{
		horizontalScrollPosition += horizontalScrollLineSize;
		ScrollWindowEx(thisWindow, -horizontalScrollLineSize, 0, nullptr, nullptr, nullptr, nullptr, 0);
	}
	else
	{
		if (horizontalScrollPosition + windowWidth < canvasHeight)	//not at the extreme right, but less than 1 line to there
		{
			int finalVertScrollDelta = canvasHeight - (horizontalScrollPosition + windowWidth);
			horizontalScrollPosition += finalVertScrollDelta;
			ScrollWindowEx(thisWindow, -finalVertScrollDelta, 0, nullptr, nullptr, nullptr, nullptr, 0);
		}
	}
}

//Invalidates the client area and sends WM_PAINT.
void RequestRepaintWindow()
{
	InvalidateRect(thisWindow, nullptr, TRUE);
	UpdateWindow(thisWindow);
}

//Clears the canvas window.
void Clear()
{
	actions.clear();
	undoneActions.clear();
	ClearBuffer();
	RequestRepaintWindow();
}

//Clears the buffer DC.
void ClearBuffer()
{
	RECT rect;
	rect.left = 0;
	rect.right = canvasWidth;
	rect.top = 0;
	rect.bottom = canvasHeight;
	FillRect(bufferDC, &rect, (HBRUSH)(COLOR_WINDOW + 1));
}

//Initializes the buffer DC.
void InitBufferDC()
{
	currentPen = CreatePen(PS_SOLID, 0, penColor);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(thisWindow, &ps);

	bufferDC = CreateCompatibleDC(nullptr);
	HBITMAP bufferBitmap = CreateCompatibleBitmap(hdc, canvasWidth, canvasHeight);
	SelectObject(bufferDC, bufferBitmap);

	COLORREF cr = GetBkColor(hdc);
	SetBkColor(bufferDC, cr);
	ClearBuffer();

	EndPaint(thisWindow, &ps);
}

//Clears the buffer and redraws from metadata.
void Refresh()
{
	RefreshBuffer();
	RequestRepaintWindow();
}

void RefreshBuffer()
{
	ClearBuffer();

	PaintAction* currShape;
	for (std::list<PaintAction*>::const_iterator iterator = actions.begin(), end = actions.end(); iterator != end; ++iterator)
	{
		currShape = *iterator;
		if (currShape != nullptr)
		{
			currShape->Draw(bufferDC, 0, 0);
		}
	}
}

void SetPaintTool(PaintTool tool)
{
	currentTool = tool;
}

void SetPenColor(COLORREF color)
{
	penColor = color;

	currentPen = CreatePen(PS_SOLID, 0, color);
	SelectObject(bufferDC, currentPen);
}

COLORREF GetPenColor()
{
	return penColor;
}

BOOL ResizeWindow(int x, int y, int width, int height)
{
	if (!MoveWindow(thisWindow, x, y, width, height, TRUE))
		return FALSE;

	ConfigureScrollBars();
	
	return TRUE;
}

void ZoomIn()
{
	double oldZoomFactor = zoomFactor;
	zoomFactor += zoomDelta;

	OnZoomFactorChanged(oldZoomFactor);
}

void ZoomOut()
{
	if (zoomFactor < zoomDelta)
		return;

	double oldZoomFactor = zoomFactor;
	zoomFactor -= zoomDelta;

	OnZoomFactorChanged(oldZoomFactor);
}

void OnZoomFactorChanged(double oldZoomFactor)
{
	verticalScrollPosition = round(verticalScrollPosition / oldZoomFactor * zoomFactor);
	horizontalScrollPosition = round(horizontalScrollPosition / oldZoomFactor * zoomFactor);

	ConfigureScrollBars();
	RequestRepaintWindow();
}

//Returns the cursor position adjusted to scroll positions and zoom factor
void GetAdjustedCursorPos(LPPOINT point)
{
	GetCursorPos(point);
	ScreenToClient(thisWindow, point);
	point->x = round((point->x + horizontalScrollPosition) / zoomFactor);
	point->y = round((point->y + verticalScrollPosition) / zoomFactor);
}

//Reverses scroll and zoom adjustments.
void AdjustedToClientPoint(LPPOINT point)
{
	point->x = round(point->x * zoomFactor - horizontalScrollPosition);
	point->y = round(point->y * zoomFactor - verticalScrollPosition);
}