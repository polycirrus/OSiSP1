#include "stdafx.h"
#include "PolygonalChain.h"

PolygonalChain::PolygonalChain()
{
}

PolygonalChain::~PolygonalChain()
{
	for (std::list<POINT*>::const_iterator iterator = vertices.begin(), end = vertices.end(); iterator != end; ++iterator)
	{
		free(*iterator);
	}
}

void PolygonalChain::Draw(HDC hdc, int xOffset, int yOffset)
{
	if (vertices.size() > 1)
	{
		std::list<POINT*>::const_iterator iterator = vertices.begin();
		MoveToEx(hdc, (*iterator)->x + xOffset, (*iterator)->y + yOffset, nullptr);

		std::list<POINT*>::const_iterator end = vertices.end();
		for (++iterator; iterator != end; ++iterator)
		{
			LineTo(hdc, (*iterator)->x + xOffset, (*iterator)->y + yOffset);
		}
	}
}

void PolygonalChain::AddVertex(int x, int y)
{
	POINT* point = (POINT*)malloc(sizeof(POINT));
	point->x = x;
	point->y = y;
	vertices.push_back(point);
}

void PolygonalChain::GetLastVertex(LPPOINT point)
{
	POINT* lastVertex = vertices.back();
	point->x = lastVertex->x;
	point->y = lastVertex->y;
}