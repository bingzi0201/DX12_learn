#include "World.h"
#include "../Engine/Engine.h"
#include <algorithm>


World::World()
{
}

void World::InitWorld(Engine* inEngine)
{
	engine = inEngine;

	mainWindowHandle = engine->GetMainWnd();
}

std::string GetToggleStateStr(bool bOn)
{
	if (bOn)
	{
		return "ON";
	}
	else
	{
		return "OFF";
	}
}

void World::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);

	// Tick actors
	for (auto& actor : actors)
	{
		actor->Tick(gt.DeltaTime());
	}

	// Calculate FPS and draw text
	{
		static float FPS = 0.0f;
		static float MSPF = 0.0f;

		static int frameCnt = 0;
		static float timeElapsed = 0.0f;
		frameCnt++;

		// Compute averages over one second period.
		if ((gt.TotalTime() - timeElapsed) >= 1.0f)
		{
			FPS = (float)frameCnt;
			MSPF = 1000.0f / FPS;

			// Reset for next average.
			frameCnt = 0;
			timeElapsed += 1.0f;
		}

		DrawString(1, "FPS: " + std::to_string(FPS), 0.1f);
		DrawString(2, "MSPF: " + std::to_string(MSPF), 0.1f);
	}


	// Print camera message
	TVector3 cameraLocation = cameraComponent->GetWorldLocation();
	DrawString(11, "CameraLocation(" + std::to_string(cameraLocation.x) + ", " + std::to_string(cameraLocation.y) + ", " + std::to_string(cameraLocation.z) + ")");

}

void World::EndFrame(const GameTimer& gt)
{
	SavePrevFrameData();

	float DeltaTime = gt.DeltaTime();

	// Clear Primitives
	points.clear();
	lines.clear();
	triangles.clear();

	// Clear Texts
	textManager.UpdateTexts(DeltaTime);
}

void World::SavePrevFrameData()
{
	for (auto& actor : actors)
	{
		if (actor->GetRootComponent())
		{
			actor->SetActorPrevTransform(actor->GetActorTransform());
		}
	}

	TMatrix view = cameraComponent->GetView();
	TMatrix proj = cameraComponent->GetProj();
	TMatrix viewProj = view * proj;
	cameraComponent->SetPrevViewProj(viewProj);
}

void World::OnMouseDown(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		LastMousePos.x = x;
		LastMousePos.y = y;

		SetCapture(mainWindowHandle);
	}
}

void World::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void World::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = 0.25f * static_cast<float>(x - LastMousePos.x);
		float dy = 0.25f * static_cast<float>(y - LastMousePos.y);

		cameraComponent->Pitch(dy);
		cameraComponent->RotateY(dx);
	}

	LastMousePos.x = x;
	LastMousePos.y = y;
}

void World::OnMouseWheel(float WheelDistance)
{
	// Positive value means rotated forward, negative value means rotated backward(toward the user)
	moveSpeed += (WheelDistance / WHEEL_DELTA);

	moveSpeed = std::clamp(moveSpeed, 1.0f, 10.0f);
}

void World::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	/*-------------Camera-------------*/
	if (GetAsyncKeyState('W') & 0x8000)
		cameraComponent->MoveForward(moveSpeed * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		cameraComponent->MoveForward(-moveSpeed * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		cameraComponent->MoveRight(-moveSpeed * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		cameraComponent->MoveRight(moveSpeed * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		cameraComponent->MoveUp(-moveSpeed * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		cameraComponent->MoveUp(moveSpeed * dt);

	cameraComponent->UpdateViewMatrix();

}

void World::DrawPoint(const TVector3& pointInWorld, const Color& color, int size)
{
	points.emplace_back(pointInWorld, color);

	if (size != 0)
	{
		float offset = 0.01f * size;

		points.emplace_back(pointInWorld + TVector3(offset, 0.0f, 0.0f), color);
		points.emplace_back(pointInWorld + TVector3(0.0f, offset, 0.0f), color);
		points.emplace_back(pointInWorld + TVector3(0.0f, 0.0f, offset), color);
		points.emplace_back(pointInWorld + TVector3(-offset, 0.0f, 0.0f), color);
		points.emplace_back(pointInWorld + TVector3(0.0f, -offset, 0.0f), color);
		points.emplace_back(pointInWorld + TVector3(0.0f, 0.0f, -offset), color);
	}
}

void World::DrawPoint(const Point& point)
{
	points.push_back(point);
}

const std::vector<Point>& World::GetPoints()
{
	return points;
}

void World::DrawLine(const TVector3& pointAInWorld, const TVector3& pointBInWorld, const Color& color)
{
	lines.emplace_back(pointAInWorld, pointBInWorld, color);
}

void World::DrawLine(const Line& line)
{
	lines.push_back(line);
}

const std::vector<Line>& World::GetLines()
{
	return lines;
}

void World::DrawBox3D(const TVector3& minPointInWorld, const TVector3& maxPointInWorld, const Color& color)
{
	TVector3 minP = minPointInWorld;
	TVector3 maxP = maxPointInWorld;

	DrawLine(TVector3(minP.x, minP.y, minP.z), TVector3(minP.x, minP.y, maxP.z), color);
	DrawLine(TVector3(minP.x, maxP.y, minP.z), TVector3(minP.x, maxP.y, maxP.z), color);
	DrawLine(TVector3(maxP.x, minP.y, minP.z), TVector3(maxP.x, minP.y, maxP.z), color);
	DrawLine(TVector3(maxP.x, maxP.y, minP.z), TVector3(maxP.x, maxP.y, maxP.z), color);

	DrawLine(TVector3(minP.x, minP.y, minP.z), TVector3(maxP.x, minP.y, minP.z), color);
	DrawLine(TVector3(minP.x, maxP.y, minP.z), TVector3(maxP.x, maxP.y, minP.z), color);
	DrawLine(TVector3(minP.x, minP.y, maxP.z), TVector3(maxP.x, minP.y, maxP.z), color);
	DrawLine(TVector3(minP.x, maxP.y, maxP.z), TVector3(maxP.x, maxP.y, maxP.z), color);

	DrawLine(TVector3(minP.x, minP.y, minP.z), TVector3(minP.x, maxP.y, minP.z), color);
	DrawLine(TVector3(maxP.x, minP.y, minP.z), TVector3(maxP.x, maxP.y, minP.z), color);
	DrawLine(TVector3(minP.x, minP.y, maxP.z), TVector3(minP.x, maxP.y, maxP.z), color);
	DrawLine(TVector3(maxP.x, minP.y, maxP.z), TVector3(maxP.x, maxP.y, maxP.z), color);
}

void World::DrawTriangle(const TVector3& pointAInWorld, const TVector3& pointBInWorld, const TVector3& pointCInWorld, const Color& color)
{
	triangles.emplace_back(pointAInWorld, pointBInWorld, pointCInWorld, color);
}

void World::DrawTriangle(const Triangle& triangle)
{
	triangles.push_back(triangle);
}

const std::vector<Triangle>& World::GetTriangles()
{
	return triangles;
}

void World::DrawSprite(const std::string& textureName, const UIntPoint& textureSize, const RECT& sourceRect, const RECT& destRect)
{
	Sprites.emplace_back(TSprite(textureName, textureSize, sourceRect, destRect));
}

const std::vector<TSprite>& World::GetSprites()
{
	return Sprites;
}

void World::DrawString(int ID, std::string str, float duration)
{
	textManager.AddText(ID, str, duration);
}

void World::GetTexts(std::vector<Text>& outTexts)
{
	textManager.GetTexts(outTexts);
}

