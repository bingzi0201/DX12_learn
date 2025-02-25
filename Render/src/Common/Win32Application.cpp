#include "stdafx.h"
#include "Win32Application.h"

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(DXApplication* pDXApp, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	pDXApp->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	// 窗口类的设计
	//开始设计一个完整的窗口类
	//用WINDCLASSEX定义了一个窗口类，即用wndClass实例化了WINCLASSEX，用于之后窗口的各项初始化
	WNDCLASSEX windowClass = { 0 };
	//设置结构体的字节数大小
	windowClass.cbSize = sizeof(WNDCLASSEX);
	//设置窗口的样式
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	//指向窗口过程函数的指针
	windowClass.lpfnWndProc = WindowProc;
	//指定包含窗口过程的程序的实例
	windowClass.hInstance = hInstance;
	//指定窗口类的光标句柄
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	//从全局的::LoadImage函数从本地加载自定义ico图标
	windowClass.hIcon = (HICON)::LoadImage(NULL, L"icon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
	//用一个以空终止的字符串，指定窗口类的名字
	windowClass.lpszClassName = L"RenderClass";

	//窗口类的注册
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(pDXApp->GetWidth()), static_cast<LONG>(pDXApp->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	//窗口的正式创建
	m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		pDXApp->GetTitle(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInstance,
		pDXApp);


	pDXApp->OnInit();

	//显示窗口
	ShowWindow(m_hwnd, SW_SHOW);

	MSG msg = {};//定义并初始化消息 
	while (msg.message != WM_QUIT)//不断从消息队列中取出消息
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);//将虚拟键消息转换为字符消息
			DispatchMessage(&msg);//分发一个消息给窗口程序
		}
	}

	pDXApp->OnDestroy();

	return static_cast<char>(msg.wParam);
}

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

//窗口过程函数
LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
	DXApplication* pDXApp = reinterpret_cast<DXApplication*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{

	case WM_CREATE:
		{
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

	case WM_PAINT:
		//pDXApp->OnUpdate();
		pDXApp->OnRender();
		return 0;

// 	case WM_LBUTTONDOWN:
// 	case WM_MBUTTONDOWN:
// 	case WM_RBUTTONDOWN:
// 		pDXApp->OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
// 		return 0;
// 
// 	case WM_LBUTTONUP:
// 	case WM_MBUTTONUP:
// 	case WM_RBUTTONUP:
// 		pDXApp->OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
// 		return 0;
// 
// 	case WM_MOUSEMOVE:
// 		pDXApp->OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
// 		return 0;

	case WM_DESTROY://窗口销毁消息
		PostQuitMessage(0);//向系统表明有个线程有终止请求。用来响应 WM_DESTROY消息
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);//调用默认的窗口过程来为应用程序没有处理的窗口消息提供默认的处理。
}