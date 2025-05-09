#pragma once

#include <vector>
#include <memory>
#include "../Actor/Actor.h"
#include "../Mesh/Color.h"
#include "../Mesh/Primitive.h"
#include "../Mesh/Sprite.h"
#include "../Mesh/TextManager.h"
#include "../Engine/GameTimer.h"
#include "../Component/CameraComponent.h"

class Engine;

class World
{
public:
	World();
	virtual ~World() {}

	virtual void InitWorld(Engine* InEngine);
	virtual void Update(const GameTimer& gt);
	virtual void EndFrame(const GameTimer& gt);

private:
	void SavePrevFrameData();

public:
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void OnMouseWheel(float WheelDistance);
	virtual void OnKeyboardInput(const GameTimer& gt);

public:
	template<typename T>
	T* AddActor(const std::string& name)
	{
		auto newActor = std::make_unique<T>(name);
		T* result = newActor.get();
		actors.push_back(std::move(newActor));

		return result;
	}

	std::vector<Actor*> GetActors()
	{
		std::vector<Actor*> result;

		for (const auto& actor : actors)
		{
			result.push_back(actor.get());
		}

		return result;
	}

	template<typename T>
	std::vector<T*> GetAllActorsOfClass()
	{
		std::vector<T*> result;

		for (const auto& actor : actors)
		{
			T* actorOfClass = dynamic_cast<T*>(actor.get());
			if (actorOfClass)
			{
				result.push_back(actorOfClass);
			}
		}

		return result;
	}

	CameraComponent* GetCameraComponent() { return cameraComponent; }

public:
	void DrawPoint(const TVector3& pointInWorld, const Color& color, int size = 0);
	void DrawPoint(const Point& point);
	const std::vector<Point>& GetPoints();

	void DrawLine(const TVector3& pointAInWorld, const TVector3& pointBInWorld, const Color& color);
	void DrawLine(const Line& line);
	const std::vector<Line>& GetLines();

	void DrawBox3D(const TVector3& minPointInWorld, const TVector3& maxPointInWorld, const Color& color);
	void DrawTriangle(const TVector3& pointAInWorld, const TVector3& pointBInWorld, const TVector3& pointCInWorld, const Color& color);
	void DrawTriangle(const Triangle& triangle);
	const std::vector<Triangle>& GetTriangles();

	void DrawSprite(const std::string& textureName, const UIntPoint& textureSize, const RECT& sourceRect, const RECT& destRect);
	const std::vector<TSprite>& GetSprites();

	void DrawString(int ID, std::string str, float duration = 1.0f);
	void GetTexts(std::vector<Text>& outTexts);

protected:
	std::vector<std::unique_ptr<Actor>> actors;
	std::vector<Point> points;
	std::vector<Line> lines;
	std::vector<Triangle> triangles;
	std::vector<TSprite> Sprites;
	TextManager textManager;

protected:
	Engine* engine = nullptr;
	HWND  mainWindowHandle = nullptr; // main window handle
	CameraComponent* cameraComponent = nullptr;
	float moveSpeed = 2.0f;

private:
	POINT LastMousePos;
	bool bKey_H_Pressed = false;
	bool bKey_J_Pressed = false;
	bool bKey_K_Pressed = false;
	bool bKey_L_Pressed = false;
};
