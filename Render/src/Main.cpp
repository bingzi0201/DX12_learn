#include "Common/stdafx.h"
#include "Common/RenderApp.h"


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	RenderApp renderApp(1280, 720, L"±ý×ÓµÄäÖÈ¾Æ÷©d(¨R¨Œ¨Q*)o");
	return Win32Application::Run(&renderApp, hInstance, nShowCmd);
}