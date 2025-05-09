#include "Engine.h"
#include <WindowsX.h>
#include <vector>
#include "../Texture/TextureRepository.h"
#include "../Material/MaterialRepository.h"
#include "../Mesh/MeshRepository.h"
#include "../World/World.h"
#include "../../resource.h"

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before MainWindowHandle is valid.
	return Engine::GetEngineSingleton()->MsgProc(hwnd, msg, wParam, lParam);
}

Engine* Engine::engineSingleton = nullptr;
Engine* Engine::GetEngineSingleton()
{
	return engineSingleton;
}

Engine::Engine(HINSTANCE HInstance)
	: engineInstHandle(HInstance)
{
	// Only one TEngine can be constructed.
	assert(engineSingleton == nullptr);
	engineSingleton = this;
}

Engine::~Engine()
{

}

HINSTANCE Engine::GetEngineInstHandle()const
{
	return engineInstHandle;
}

HWND Engine::GetMainWnd()const
{
	return mainWindowHandle;
}

bool Engine::Initialize(World* inWorld, const TRenderSettings& renderSettings)
{
	if (!InitMainWindow())
		return false;

	d3d12RHI = std::make_unique<D3D12RHI>();
	d3d12RHI->Initialize(mainWindowHandle, windowWidth, windowHeight);

	TextureRepository::Get().Load();
	MaterialRepository::Get().Load();
	MeshRepository::Get().Load();

	world.reset(inWorld);
	world->InitWorld(this);

	render = std::make_unique<Render>();
	if (!render->Initialize(windowWidth, windowHeight, d3d12RHI.get(), world.get(), renderSettings))
		return false;

	bInitialize = true;

	return true;
}

int Engine::Run()
{
	MSG msg = { 0 };

	timer.Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			timer.Tick();

			if (!bAppPaused)
			{
				CalculateFrameStats();

				Update(timer);

				EndFrame(timer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool Engine::Destroy()
{
	render.reset();

	world.reset();

	TextureRepository::Get().Unload();
	MaterialRepository::Get().Unload();
	MeshRepository::Get().Unload();

	d3d12RHI.reset();

	return true;
}


LRESULT Engine::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!bInitialize)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			bAppPaused = true;
			timer.Stop();
		}
		else
		{
			bAppPaused = false;
			timer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		windowWidth = LOWORD(lParam);
		windowHeight = HIWORD(lParam);

		if (wParam == SIZE_MINIMIZED)
		{
			bAppPaused = true;
			bAppMinimized = true;
			bAppMaximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			bAppPaused = false;
			bAppMinimized = false;
			bAppMaximized = true;
			OnResize();
		}
		else if (wParam == SIZE_RESTORED)
		{

			// Restoring from minimized state?
			if (bAppMinimized)
			{
				bAppPaused = false;
				bAppMinimized = false;
				OnResize();
			}

			// Restoring from maximized state?
			else if (bAppMaximized)
			{
				bAppPaused = false;
				bAppMaximized = false;
				OnResize();
			}
			else if (bResizing)
			{
				// If user is dragging the resize bars, we do not resize 
				// the buffers here because as the user continuously 
				// drags the resize bars, a stream of WM_SIZE messages are
				// sent to the window, and it would be pointless (and slow)
				// to resize for each WM_SIZE message received from dragging
				// the resize bars.  So instead, we reset after the user is 
				// done resizing the window and releases the resize bars, which 
				// sends a WM_EXITSIZEMOVE message.
			}
			else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
			{
				OnResize();
			}
		}

		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		bAppPaused = true;
		bResizing = true;
		timer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		bAppPaused = false;
		bResizing = false;
		timer.Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEWHEEL:
		OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool Engine::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = engineInstHandle;
	wc.hIcon = LoadIcon(engineInstHandle, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, windowWidth, windowHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mainWindowHandle = CreateWindow(L"MainWnd", windowTile.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, engineInstHandle, 0);
	if (!mainWindowHandle)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mainWindowHandle, SW_SHOW);
	UpdateWindow(mainWindowHandle);

	return true;
}

void Engine::OnResize()
{
	if (render->IsInitialize()) {
		render->OnResize(windowWidth, windowHeight);
	}
}

void Engine::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((timer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

		std::wstring windowText = windowTile +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mainWindowHandle, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void Engine::Update(const GameTimer& gt)
{
	world->Update(timer);

	render->Draw(timer);
}

void Engine::EndFrame(const GameTimer& gt)
{
	world->EndFrame(gt);

	render->EndFrame();
}
