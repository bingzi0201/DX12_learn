#pragma once

#include <memory>
#include "Component.h"
#include "../Mesh/Mesh.h"
#include "../Material/MaterialInstance.h"

class MeshComponent : public Component
{
public:
	void SetMeshName(std::string InMeshName);
	std::string GetMeshName() const;

	bool IsMeshValid() const;

	bool GetLocalBoundingBox(TBoundingBox& OutBox);
	bool GetWorldBoundingBox(TBoundingBox& OutBox);

	void SetMaterialInstance(std::string MaterialInstanceName);
	MaterialInstance* GetMaterialInstance() { return materialInstance; }

public:
	TMatrix TexTransform = TMatrix::Identity;

	// Flags
	bool bUseSDF = false;

private:
	std::string meshName;

	MaterialInstance* materialInstance;
};