#include "stdafx.h"
#include "EllipseAction.h"


EllipseAction::EllipseAction(LPPOINT point)
{
	p1.x = point->x;
	p1.y = point->y;
	p2.x = point->x;
	p2.y = point->y;
}

EllipseAction::~EllipseAction()
{
}

void EllipseAction::GetP1(LPPOINT point)
{
	point->x = p1.x;
	point->y = p1.y;
}

void EllipseAction::GetP2(LPPOINT point)
{
	point->x = p2.x;
	point->y = p2.y;
}

void EllipseAction::SetP2(LPPOINT point)
{
	p2.x = point->x;
	p2.y = point->y;
}

void EllipseAction::Draw(HDC hdc, int xOffset, int yOffset)
{
	Ellipse(hdc, p1.x, p1.y, p2.x, p2.y);
}