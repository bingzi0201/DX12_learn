#pragma once

#include <unordered_map>
#include <string>
#include "Material.h"
#include "MaterialInstance.h"

class MaterialRepository
{
public:
	static MaterialRepository& Get();

	void Load();

	void Unload();

	MaterialInstance* GetMaterialInstance(const std::string& MaterialInstanceName) const;

private:
	Material* CreateMaterial(const std::string& MaterialName, const std::string& ShaderName);

	MaterialInstance* CreateMaterialInstance(Material* material, const std::string& MaterialInstanceName);

	void CreateDefaultMaterialInstance(Material* material);

public:
	std::unordered_map<std::string /*MaterialName*/, std::unique_ptr<Material>> materialMap;

	std::unordered_map<std::string /*MaterialInstanceName*/, std::unique_ptr<MaterialInstance>> materialInstanceMap;
};
