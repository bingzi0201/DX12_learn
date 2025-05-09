#include "StaticMeshActor.h"


StaticMeshActor::StaticMeshActor(const std::string& Name)
	:Actor(Name)
{
	StaticMeshComponent = AddComponent<MeshComponent>();

	rootComponent = StaticMeshComponent;
}
StaticMeshActor::~StaticMeshActor()
{}

void StaticMeshActor::SetMesh(std::string MeshName)
{
	StaticMeshComponent->SetMeshName(MeshName);
}

void StaticMeshActor::SetMaterialInstance(std::string MaterialInstanceName)
{
	StaticMeshComponent->SetMaterialInstance(MaterialInstanceName);
}

void StaticMeshActor::SetTextureScale(const TVector2& Scale)
{
	StaticMeshComponent->TexTransform = TMatrix::CreateScale(Scale.x, Scale.y, 1.0f);
}

void StaticMeshActor::SetUseSDF(bool bUseSDF)
{
	StaticMeshComponent->bUseSDF = bUseSDF;
}