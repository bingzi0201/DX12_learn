#pragma once

#include "../Math/Math.h"
#include <string>

struct Text
{
	Text() {}
	Text(int inID, const std::string& inContent, const UIntPoint& inScreenPos, float inDuration)
		:ID(inID), content(inContent), screenPos(inScreenPos), duration(inDuration)
	{}

	std::string content;
	UIntPoint screenPos;

	int ID = -1;
	float duration = 0.0f;  // in seconds
};