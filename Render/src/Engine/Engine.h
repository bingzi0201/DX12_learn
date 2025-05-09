#pragma once

#include "GameTimer.h"
#include "../World/World.h"
#include "../Render/Render.h"
#include "../Resource/D3D12RHI.h"

class Engine
{
public:
	Engine(HINSTANCE HInstance);
	Engine(const Engine& rhs) = delete;
	Engine& operator=(const Engine& rhs) = delete;

	virtual ~Engine();

public:
	static Engine* GetEngineSingleton();
	HINSTANCE GetEngineInstHandle()const;
	HWND      GetMainWnd()const;

	bool Initialize(World* InWorld, const TRenderSettings& renderSettings);
	int Run();
	bool Destroy();
	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	int GetWindowWidth() { return windowWidth; }
	int GetWindowHeight() { return windowHeight; }
	Render* GetRender() { return render.get(); }

private:
	bool InitMainWindow();
	void OnResize();
	void CalculateFrameStats();

protected:
	void Update(const GameTimer& gt);
	void EndFrame(const GameTimer& gt);

	void OnMouseDown(WPARAM btnState, int x, int y) { world->OnMouseDown(btnState, x, y); }
	void OnMouseUp(WPARAM btnState, int x, int y) { world->OnMouseUp(btnState, x, y); }
	void OnMouseMove(WPARAM btnState, int x, int y) { world->OnMouseMove(btnState, x, y); }
	void OnMouseWheel(float WheelDistance) { world->OnMouseWheel(WheelDistance); }

protected:
	static Engine* engineSingleton;
	std::wstring windowTile = L"饼子的渲染器ヾ(•ω•`)o";

	bool bInitialize = false;

	HINSTANCE engineInstHandle = nullptr;
	HWND      mainWindowHandle = nullptr;
	bool      bAppPaused = false;
	bool      bAppMinimized = false;
	bool      bAppMaximized = false; 
	bool      bResizing = false;
	bool      bFullscreenState = false;

	int windowWidth = 1280;
	int windowHeight = 720;

	GameTimer timer;

	std::unique_ptr<D3D12RHI> d3d12RHI;
	std::unique_ptr<World> world;
	std::unique_ptr<Render> render;
};

