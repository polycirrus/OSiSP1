#pragma once
#include "PaintAction.h"
class EllipseAction :
	public PaintAction
{
public:
	EllipseAction(LPPOINT point);
	~EllipseAction();
	void Draw(HDC hdc, int xOffset, int yOffset);
	void GetP1(LPPOINT point);
	void GetP2(LPPOINT point);
	void SetP2(LPPOINT point);
private:
	POINT p1, p2;
};

