#pragma once

#include "DXHelper.h"
#include "Win32Application.h"

class DXApplication
{
public:
	DXApplication(uint32_t width, uint32_t height, std::wstring name);
	virtual ~DXApplication();

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	// Samples override the event handlers to handle specific messages.
	virtual void OnKeyDown(UINT8 /*key*/) {}
	virtual void OnKeyUp(UINT8 /*key*/) {}

	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	const wchar_t* GetTitle() const { return m_title.c_str(); }

	void ParseCommandLineArgs(_In_reads_(argc) wchar_t* argv[], int argc);

protected:
	std::wstring GetAssetFullPath(LPCWSTR assetName);

	void GetHardwareAdapter(
		_In_ IDXGIFactory1* pFactory,
		_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	void SetCustomWindowText(LPCWSTR text);

	uint32_t m_width;
	uint32_t m_height;
	float m_aspectRatio;

	bool m_useWarpDevice;

private:
	std::wstring m_assetsPath;
	std::wstring m_title;
};
