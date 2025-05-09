#pragma once

#include <string>
#include <filesystem>
#include "../Utils/FormatConvert.h"

class TFileHelpers
{
public:
	static bool IsFileExit(const std::wstring& FileName)
	{
		return std::filesystem::exists(FileName);
	}
};
