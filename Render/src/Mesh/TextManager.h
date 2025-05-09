#pragma once

#include "Text.h"
#include <list>
#include <vector>

class TextManager
{
public:
	void AddText(int ID, std::string str, float duration);
	void GetTexts(std::vector<Text>& outTexts);
	void UpdateTexts(float DeltaTime);

private:
	std::list<Text> textList;
	int minGeneratedTextID = 10000;
	int currentGeneratedTextID = -1;
	UIntPoint textStartPos = UIntPoint(30, 30);
	uint32_t textHeightOffset = 20;
};
