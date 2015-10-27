#include "stdafx.h"
#include "Rectangle.h"


RectangleAction::RectangleAction(LPPOINT point)
{
	p1.x = point->x;
	p1.y = point->y;
	p2.x = point->x;
	p2.y = point->y;
}

void RectangleAction::GetP1(LPPOINT point)
{
	point->x = p1.x;
	point->y = p1.y;
}

void RectangleAction::GetP2(LPPOINT point)
{
	point->x = p2.x;
	point->y = p2.y;
}

void RectangleAction::SetP2(LPPOINT point)
{
	p2.x = point->x;
	p2.y = point->y;
}

RectangleAction::~RectangleAction()
{
}

void RectangleAction::Draw(HDC hdc, int xOffset, int yOffset)
{
	Rectangle(hdc, p1.x, p1.y, p2.x, p2.y);
}