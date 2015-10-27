#pragma once
#include "PaintAction.h"
#include "stdafx.h"
#include <list>

class PolygonalChain :
	public PaintAction
{
public:
	PolygonalChain();
	~PolygonalChain();
	void Draw(HDC hdc, int xOffset, int yOffset);
	void AddVertex(int x, int y);
	void GetLastVertex(LPPOINT point);
	void SetLastVertex(LPPOINT point);
	void GetSecondLastVertex(LPPOINT point);
private:
	std::list<POINT*> vertices;
};

