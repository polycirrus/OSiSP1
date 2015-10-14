#include "stdafx.h"
#include "Line.h"

Line::Line()
{
	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
}

Line::Line(int x1, int y1, int x2, int y2)
{
	Line::x1 = x1;
	Line::y1 = y1;
	Line::x2 = x2;
	Line::y2 = y2;
}

Line::~Line()
{
}

void Line::Draw(HDC hdc, int xOffset, int yOffset)
{
	MoveToEx(hdc, x1 + xOffset, y1 + yOffset, nullptr);
	LineTo(hdc, x2 + xOffset, y2 + yOffset);
}