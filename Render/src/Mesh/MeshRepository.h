#pragma once

#include <unordered_map>
#include <string>
#include "Mesh.h"
#include "MeshLoader.h"

class MeshRepository
{
public:
	MeshRepository();
	static MeshRepository& Get();
	void Load();
	void Unload();

public:
	std::unordered_map<std::string, Mesh> meshMap;

private:
	std::unique_ptr<MeshLoader> meshLoader;
};