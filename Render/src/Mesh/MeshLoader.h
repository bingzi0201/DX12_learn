#pragma once

#include "Assimp/Importer.hpp"
#include "Assimp/scene.h"
#include "Assimp/postprocess.h"
#include <memory>
#include <string>
#include "Mesh.h"

class MeshLoader
{
public:
	void Init();
	bool LoadModel(const std::wstring& modelName, Mesh& mesh);

private:
	bool ProcessMesh(const aiMesh* assimpMesh, Mesh& mesh, const aiMatrix4x4& vertexTransform, const aiMatrix4x4& normalTransform);
	void ProcessNode(const aiNode* node, Mesh& mesh, const aiMatrix4x4& parentVertexTransform, const aiMatrix4x4& parentNormalTransform);
	aiMatrix4x4 GetNodeTransform(const aiNode* node);

	// Debug
	void PrintNodeHierarchy(const aiNode* node, int depth);

private:
	Assimp::Importer m_importer;
	bool m_bDebugMode = false;
};