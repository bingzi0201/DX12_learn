#pragma once

#include "Color.h"
#include "../Math/Math.h"
#include "../Resource/Buffer.h"

struct Vertex
{
	Vertex() {}
	Vertex(
		const TVector3& p,
		const TVector3& n,
		const TVector3& t,
		const TVector2& uv) :
		position(p),
		normal(n),
		tangentU(t),
		texcoord(uv) {}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :
		position(px, py, pz),
		normal(nx, ny, nz),
		tangentU(tx, ty, tz),
		texcoord(u, v) {}

	TVector3 position;
	TVector3 normal;
	TVector3 tangentU;
	TVector2 texcoord;
};

struct PrimitiveVertex
{
	PrimitiveVertex() {}
	PrimitiveVertex(
		const TVector3& p,
		const Color& c) :
		Position(p),
		color(c) {}

	PrimitiveVertex(
		float px, float py, float pz,
		float cr, float cg, float cb, float ca) :
		Position(px, py, pz),
		color(cr, cg, cb, ca) {}

	TVector3 Position;
	Color color;
};

struct SpriteVertex
{
	SpriteVertex() {}
	SpriteVertex(
		const TVector3& p,
		const TVector2& uv) :
		Position(p),
		TexC(uv) {}

	TVector3 Position;
	TVector2 TexC;
};