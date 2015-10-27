#include "stdafx.h"
#include "Canvas.h"
#include <list>
#include "Line.h"
#include "PolygonalChain.h"
#include <ctgmath>
#include "MetafilePlayback.h"
#include "Rectangle.h"
#include "EllipseAction.h"


HINSTANCE hInstance;
HWND parentWindow, thisWindow;
LPCWSTR thisClassName = L"canvas";
LPCWSTR thisWindowName = L"Canvas";
bool isWindowCreated;

int canvasWidth = 2000;
int canvasHeight = 2000;
HDC bufferDC = nullptr;

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
bool polylineDragging;

PaintTool currentTool = PaintTool::None;
std::list<PaintAction*> actions;
std::list<PaintAction*> undoneActions;

COLORREF penColor = RGB(0, 0, 0);
COLORREF brushColor = RGB(255, 255, 255);
HPEN currentPen;
HPEN eraser;
HBRUSH currentBrush;
int penThickness = 1;
int penThicknessDelta = 2;


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
void				OnRButtonDown();
void				ResetPen();
void				ResetBrush();


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
		OnLButtonDown();
		break;
	case WM_MOUSELEAVE:
	case WM_LBUTTONUP:
		OnLButtonUp();
		break;
	case WM_MOUSEMOVE:
		OnMouseMove(lParam);
		break;
	case WM_RBUTTONDOWN:
		OnRButtonDown();
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

	if (downKeys & MK_CONTROL)		//Ctrl+Wheel - zoom
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
	case PaintTool::Polyline:
	{
		PolygonalChain* pl;
		if (!polylineDragging)
		{
			polylineDragging = true;
			pl = new PolygonalChain();
			actions.push_back(pl);
		}
		else
		{
			pl = (PolygonalChain*)actions.back();
			pl->Draw(bufferDC, 0, 0);
		}
		pl->AddVertex(mousePos.x, mousePos.y);
		pl->AddVertex(mousePos.x, mousePos.y);
	}
		break;
	case PaintTool::Rectangle:
	{
		RectangleAction* rect = new RectangleAction(&mousePos);
		actions.push_back(rect);
	}
		break;
	case PaintTool::Ellipse:
	{
		EllipseAction* ellipse = new EllipseAction(&mousePos);
		actions.push_back(ellipse);
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
	case PaintTool::Rectangle:
	case PaintTool::Ellipse:
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

		if (mousePos.x <= canvasWidth && mousePos.y <= canvasHeight)
		{
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
			case PaintTool::Rectangle:
			{
				RectangleAction* rect = (RectangleAction*)actions.back();

				POINT P1, prevP2;
				rect->GetP1(&P1);
				rect->GetP2(&prevP2);
				rect->SetP2(&mousePos);

				AdjustedToClientPoint(&P1);
				AdjustedToClientPoint(&prevP2);
				AdjustedToClientPoint(&mousePos);
				invRect.bottom = max(max(P1.y, prevP2.y), mousePos.y) + invalidationExcess;
				invRect.top = min(min(P1.y, prevP2.y), mousePos.y) - invalidationExcess;
				invRect.left = min(min(P1.x, prevP2.x), mousePos.x) - invalidationExcess;
				invRect.right = max(max(P1.x, prevP2.x), mousePos.x) + invalidationExcess;
			}
			break;
			case PaintTool::Ellipse:
			{
				EllipseAction* ellipse = (EllipseAction*)actions.back();

				POINT P1, prevP2;
				ellipse->GetP1(&P1);
				ellipse->GetP2(&prevP2);
				ellipse->SetP2(&mousePos);

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
		}

		InvalidateRect(thisWindow, &invRect, TRUE);
		UpdateWindow(thisWindow);
	}

	if (polylineDragging)
	{
		RECT invRect;
		POINT mousePos;
		GetAdjustedCursorPos(&mousePos);

		PolygonalChain* pl = (PolygonalChain*)actions.back();

		POINT P1, prevP2;
		pl->GetSecondLastVertex(&P1);
		pl->GetLastVertex(&prevP2);
		pl->SetLastVertex(&mousePos);

		AdjustedToClientPoint(&P1);
		AdjustedToClientPoint(&prevP2);
		AdjustedToClientPoint(&mousePos);
		invRect.bottom = max(max(P1.y, prevP2.y), mousePos.y) + invalidationExcess;
		invRect.top = min(min(P1.y, prevP2.y), mousePos.y) - invalidationExcess;
		invRect.left = min(min(P1.x, prevP2.x), mousePos.x) - invalidationExcess;
		invRect.right = max(max(P1.x, prevP2.x), mousePos.x) + invalidationExcess;

		InvalidateRect(thisWindow, &invRect, TRUE);
		UpdateWindow(thisWindow);
	}
}


void OnRButtonDown()
{
	if (polylineDragging)
	{
		polylineDragging = false;
		actions.back()->Draw(bufferDC, 0, 0);
	}
}

//Handles the paint message (WM_PAINT).
void OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(thisWindow, &ps);
	SelectObject(hdc, currentPen);
	SelectObject(hdc, currentBrush);

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
			POINT p1, p2;
			Line* line = (Line*)actions.back();
			line->GetP1(&p1);
			line->GetP2(&p2);
			AdjustedToClientPoint(&p1);
			AdjustedToClientPoint(&p2);

			MoveToEx(hdc, p1.x, p1.y, nullptr);
			LineTo(hdc, p2.x, p2.y);
		}
		break;
		case PaintTool::Rectangle:
		{
			POINT p1, p2;
			RectangleAction* rect = (RectangleAction*)actions.back();
			rect->GetP1(&p1);
			rect->GetP2(&p2);
			AdjustedToClientPoint(&p1);
			AdjustedToClientPoint(&p2);

			Rectangle(hdc, p1.x, p1.y, p2.x, p2.y);
		}
			break;
		case PaintTool::Ellipse:
		{
			POINT p1, p2;
			EllipseAction* ellipse = (EllipseAction*)actions.back();
			ellipse->GetP1(&p1);
			ellipse->GetP2(&p2);
			AdjustedToClientPoint(&p1);
			AdjustedToClientPoint(&p2);

			Ellipse(hdc, p1.x, p1.y, p2.x, p2.y);
		}
		break;
		default:
			break;
		}
	}

	if (polylineDragging)
	{
		POINT p1, p2;
		PolygonalChain* pl = (PolygonalChain*)actions.back();
		pl->GetSecondLastVertex(&p1);
		pl->GetLastVertex(&p2);
		AdjustedToClientPoint(&p1);
		AdjustedToClientPoint(&p2);

		MoveToEx(hdc, p1.x, p1.y, nullptr);
		LineTo(hdc, p2.x, p2.y);
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
	if (bufferDC)
		DeleteDC(bufferDC);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(thisWindow, &ps);

	bufferDC = CreateCompatibleDC(nullptr);
	HBITMAP bufferBitmap = CreateCompatibleBitmap(hdc, canvasWidth, canvasHeight);
	SelectObject(bufferDC, bufferBitmap);

	COLORREF cr = GetBkColor(hdc);
	SetBkColor(bufferDC, cr);
	ClearBuffer();

	ResetPen();
	ResetBrush();

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
	ResetPen();
}

COLORREF GetPenColor()
{
	return penColor;
}

void SetBrushColor(COLORREF color)
{
	brushColor = color;
	ResetBrush();
}

COLORREF GetBrushColor()
{
	return brushColor;
}

void IncreasePenThickness()
{
	penThickness += penThicknessDelta;
	ResetPen();
}

void DecreasePenThickness()
{
	if (penThickness > penThicknessDelta)
	{
		penThickness -= penThicknessDelta;
		ResetPen();
	}
}

void ResetPen()
{
	currentPen = CreatePen(PS_SOLID, penThickness, penColor);
	eraser = CreatePen(PS_SOLID, penThickness, GetSysColor(COLOR_WINDOW));
	SelectObject(bufferDC, currentPen);
}

void ResetBrush()
{
	currentBrush = CreateSolidBrush(brushColor);
	SelectObject(bufferDC, currentBrush);
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

void LoadFromMetafile(LPCWSTR filename)
{
	HENHMETAFILE hMetafile = GetEnhMetaFile(filename);

	HDC dc = bufferDC ? bufferDC : GetDC(thisWindow);
	int iWidthMM = GetDeviceCaps(dc, HORZSIZE);
	int iHeightMM = GetDeviceCaps(dc, VERTSIZE);
	int iWidthPels = GetDeviceCaps(dc, HORZRES);
	int iHeightPels = GetDeviceCaps(dc, VERTRES);
	if (!bufferDC)
		ReleaseDC(thisWindow, dc);

	ENHMETAHEADER metafileHeader;
	GetEnhMetaFileHeader(hMetafile, sizeof(ENHMETAHEADER), &metafileHeader);

	canvasWidth = round(metafileHeader.rclFrame.right / (double)iWidthMM * (double)iWidthPels / 100);
	canvasHeight = round(metafileHeader.rclFrame.bottom / (double)iHeightMM * (double)iHeightPels / 100);

	InitBufferDC();

	actions.clear();
	actions.push_back(new MetafilePlayback(hMetafile));

	RefreshBuffer();
	RequestRepaintWindow();
}

void SaveToMetafile(LPCWSTR filename)
{
	int iWidthMM = GetDeviceCaps(bufferDC, HORZSIZE);
	int iHeightMM = GetDeviceCaps(bufferDC, VERTSIZE);
	int iWidthPels = GetDeviceCaps(bufferDC, HORZRES);
	int iHeightPels = GetDeviceCaps(bufferDC, VERTRES);

	RECT boundingRect;
	boundingRect.top = 0;
	boundingRect.bottom = round(canvasHeight * 100 * (double)iHeightMM / (double)iHeightPels);
	boundingRect.left = 0;
	boundingRect.right = round(canvasWidth * 100 * (double)iWidthMM / (double)iWidthPels);

	HDC hMetafileDC = CreateEnhMetaFile(nullptr, filename, &boundingRect, nullptr);

	PaintAction* currShape;
	for (std::list<PaintAction*>::const_iterator iterator = actions.begin(), end = actions.end(); iterator != end; ++iterator)
	{
		currShape = *iterator;
		if (currShape != nullptr)
		{
			currShape->Draw(hMetafileDC, 0, 0);
		}
	}

	HENHMETAFILE hMetafile = CloseEnhMetaFile(hMetafileDC);
	DeleteEnhMetaFile(hMetafile);
}

BOOL Undo()
{
	if (actions.empty())
		return FALSE;

	PaintAction* action = actions.back();
	actions.pop_back();
	undoneActions.push_back(action);

	RefreshBuffer();
	RequestRepaintWindow();

	return TRUE;
}

BOOL Redo()
{
	if (undoneActions.empty())
		return FALSE;

	PaintAction* action = undoneActions.back();
	undoneActions.pop_back();
	actions.push_back(action);

	RefreshBuffer();
	RequestRepaintWindow();

	return TRUE;
}