#pragma once

#include "Actor.h"
#include "../Component/MeshComponent.h"

class StaticMeshActor : public Actor
{
public:
	StaticMeshActor(const std::string& Name);

	~StaticMeshActor();

	void SetMesh(std::string MeshName);

	void SetMaterialInstance(std::string MaterialInstanceName);

	void SetTextureScale(const TVector2& Scale);

	void SetUseSDF(bool bUseSDF);

private:
	MeshComponent* StaticMeshComponent = nullptr;
};
