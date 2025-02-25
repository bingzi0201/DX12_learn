#pragma once
#include "DXApplication.h"

class DXApplication;

class Win32Application
{
public:
	static int Run(DXApplication* pDXApp, HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);
private:
	static HWND m_hwnd;
};
