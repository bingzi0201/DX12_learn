#include "Common/stdafx.h"
#include "Common/RenderApp.h"


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	RenderApp renderApp(1280, 720, L"���ӵ���Ⱦ���d(�R���Q*)o");
	return Win32Application::Run(&renderApp, hInstance, nShowCmd);
}