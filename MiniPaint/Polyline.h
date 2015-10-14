#pragma once
#include "Shape.h"
#include "stdafx.h"
#include <list>

class PolygonalChain :
	public Shape
{
public:
	PolygonalChain();
	~PolygonalChain();
	void Draw(HDC hdc, int xOffset, int yOffset);
	void AddVertex(int x, int y);
private:
	std::list<POINT*> vertices;
};

