#include "TextManager.h"



void TextManager::AddText(int ID, std::string Str, float Duration)
{
	assert(ID < minGeneratedTextID);

	if (ID == -1) // Add new text with auto generated ID
	{
		if (currentGeneratedTextID == -1)
		{
			currentGeneratedTextID = minGeneratedTextID;
		}

		ID = currentGeneratedTextID;
		currentGeneratedTextID++;

		textList.push_back(Text(ID, Str, UIntPoint(), Duration));
	}
	else
	{
		bool bHasID = false;

		for (auto& Text : textList)
		{
			if (ID == Text.ID) // Assign new value
			{
				Text.content = Str;
				Text.duration = Duration;

				bHasID = true;
				break;
			}
		}

		if (!bHasID) // Add new text with the given ID
		{
			textList.push_back(Text(ID, Str, UIntPoint(), Duration));
		}
	}
}

void TextManager::GetTexts(std::vector<Text>& OutTexts)
{
	UIntPoint TextPos = textStartPos;

	for (auto& Text : textList)
	{
		Text.screenPos = TextPos;

		TextPos.y += textHeightOffset;

		OutTexts.push_back(Text);
	}
}

void TextManager::UpdateTexts(float DeltaTime)
{
	auto Iter = textList.begin();
	while (Iter != textList.end())
	{
		Iter->duration -= DeltaTime;

		if (Iter->duration <= 0.0f) // Remove expired texts
		{
			Iter = textList.erase(Iter);
		}
		else
		{
			Iter++;
		}
	}
}