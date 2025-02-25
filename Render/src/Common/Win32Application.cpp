#include "stdafx.h"
#include "Win32Application.h"

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(DXApplication* pDXApp, HINSTANCE hInstance, int nCmdShow)
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	pDXApp->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	// ����������
	//��ʼ���һ�������Ĵ�����
	//��WINDCLASSEX������һ�������࣬����wndClassʵ������WINCLASSEX������֮�󴰿ڵĸ����ʼ��
	WNDCLASSEX windowClass = { 0 };
	//���ýṹ����ֽ�����С
	windowClass.cbSize = sizeof(WNDCLASSEX);
	//���ô��ڵ���ʽ
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	//ָ�򴰿ڹ��̺�����ָ��
	windowClass.lpfnWndProc = WindowProc;
	//ָ���������ڹ��̵ĳ����ʵ��
	windowClass.hInstance = hInstance;
	//ָ��������Ĺ����
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	//��ȫ�ֵ�::LoadImage�����ӱ��ؼ����Զ���icoͼ��
	windowClass.hIcon = (HICON)::LoadImage(NULL, L"icon.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
	//��һ���Կ���ֹ���ַ�����ָ�������������
	windowClass.lpszClassName = L"RenderClass";

	//�������ע��
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(pDXApp->GetWidth()), static_cast<LONG>(pDXApp->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	//���ڵ���ʽ����
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

	//��ʾ����
	ShowWindow(m_hwnd, SW_SHOW);

	MSG msg = {};//���岢��ʼ����Ϣ 
	while (msg.message != WM_QUIT)//���ϴ���Ϣ������ȡ����Ϣ
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);//���������Ϣת��Ϊ�ַ���Ϣ
			DispatchMessage(&msg);//�ַ�һ����Ϣ�����ڳ���
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

//���ڹ��̺���
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

	case WM_DESTROY://����������Ϣ
		PostQuitMessage(0);//��ϵͳ�����и��߳�����ֹ����������Ӧ WM_DESTROY��Ϣ
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);//����Ĭ�ϵĴ��ڹ�����ΪӦ�ó���û�д���Ĵ�����Ϣ�ṩĬ�ϵĴ���
}