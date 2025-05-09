#pragma once

#include <string>
#include <vector>
#include <cstdio>

class TBinarySaver
{
public:
	TBinarySaver(const std::wstring& InFilePath);

	template<typename T>
	bool Save(T Data)
	{
		return SaveArray<T>(&Data, 1);
	}

	template<typename T>
	bool SaveArray(T* Data, size_t ElementCount)
	{
		FILE* fp = _wfopen(FilePath.c_str(), L"ab");
		if (fp)
		{
			fwrite(Data, sizeof(T), ElementCount, fp);
			fclose(fp);
			return true;
		}
		return false;
	}


private:
	std::wstring FilePath;
};
