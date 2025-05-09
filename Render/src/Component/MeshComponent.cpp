#include "MeshComponent.h"
#include "../Material/MaterialRepository.h"
#include "../Mesh/MeshRepository.h"

void MeshComponent::SetMeshName(std::string inMeshName)
{
	meshName = inMeshName;
}

std::string MeshComponent::GetMeshName() const
{
	return meshName;
}

bool MeshComponent::IsMeshValid() const
{
	return (meshName != "");
}

bool MeshComponent::GetLocalBoundingBox(TBoundingBox& outBox)
{
	Mesh& mesh = MeshRepository::Get().meshMap.at(meshName);
	TBoundingBox box = mesh.GetBoundingBox();

	if (box.bInit)
	{
		outBox = box;

		return true;
	}
	else
	{
		return false;
	}
}

bool MeshComponent::GetWorldBoundingBox(TBoundingBox& outBox)
{
	TBoundingBox localBox;

	if (GetLocalBoundingBox(localBox))
	{
		outBox = localBox.Transform(worldTransform);

		return true;
	}
	else
	{
		return false;
	}
}

void MeshComponent::SetMaterialInstance(std::string materialInstanceName)
{
	materialInstance = MaterialRepository::Get().GetMaterialInstance(materialInstanceName);

	assert(materialInstance);  //TODO
}