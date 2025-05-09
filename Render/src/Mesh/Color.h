#pragma once

struct Color 
{
public:
	Color() = default;
	Color(float Value)
		: R(Value), G(Value), B(Value), A(Value) 
	{}
	Color(float R, float G, float B, float A = 1.0f) 
		: R(R), G(G), B(B), A(A) 
	{}

public:
	static const Color Black;
	static const Color White;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Magenta;

public:
	float R = 0;
	float G = 0;
	float B = 0;
	float A = 0;
};