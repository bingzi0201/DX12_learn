#include "MeshLoader.h"
#include <iostream>

void MeshLoader::Init()
{
	m_bDebugMode = false;
}

bool MeshLoader::LoadModel(const std::wstring& modelName, Mesh& mesh)
{
	// building direction
	std::wstring modelDir = L"Resources\\Models\\";
	std::wstring modelPath = modelDir + modelName;
	std::string modelPath_UTF8(modelPath.begin(), modelPath.end());

	// load models
	const aiScene* scene = m_importer.ReadFile(
		modelPath_UTF8,
		aiProcess_Triangulate |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_FlipUVs);

	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
	{
		std::cerr << "Assimp Error: " << m_importer.GetErrorString() << std::endl;
		return false;
	}

	aiMatrix4x4 scaleMatrix;
	if (scene->mMetaData && scene->mMetaData->Get("UnitScaleFactor", scaleMatrix.a1))
	{
		scaleMatrix = aiMatrix4x4(scaleMatrix).Inverse(); // 转换为米单位
	}

	ProcessNode(scene->mRootNode, mesh, scaleMatrix, scaleMatrix.Inverse().Transpose());

	// assert model
	assert(mesh.vertices.size() > 0 && "model vertex is 0");
	assert(mesh.indices32.size() > 0 && "model index is 0");

	mesh.GenerateIndices16();

	// Debug print nodes
	if (m_bDebugMode)
	{
		PrintNodeHierarchy(scene->mRootNode,0);
	}

	return true;
}

void MeshLoader::ProcessNode(const aiNode* node, Mesh& mesh, const aiMatrix4x4& parentVertexTransform, const aiMatrix4x4& parentNormalTransform)
{
	// compute vertices transform
	aiMatrix4x4 nodeVertexTransform = parentVertexTransform * GetNodeTransform(node);

	// compute normal transform
	aiMatrix4x4 nodeNormalTransform = nodeVertexTransform;
	nodeNormalTransform.Inverse().Transpose();

	// process all meshes
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		const aiMesh* assimpMesh = m_importer.GetScene()->mMeshes[node->mMeshes[i]];
		if (!ProcessMesh(assimpMesh, mesh, nodeVertexTransform, nodeNormalTransform))
			throw std::runtime_error("Failed to process mesh");
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(node->mChildren[i], mesh, nodeVertexTransform, nodeNormalTransform);
	}

}

bool MeshLoader::ProcessMesh(const aiMesh* assimpMesh, Mesh& mesh, const aiMatrix4x4& vertexTransform, const aiMatrix4x4& normalTransform)
{
	// check if triangle mesh
	if (assimpMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
		return false;
	}

	const uint32_t BaseVertex = static_cast<uint32_t>(mesh.vertices.size());
	for (unsigned int v = 0; v < assimpMesh->mNumVertices; v++) {
		Vertex vertex;

		// position
		aiVector3D pos = vertexTransform * assimpMesh->mVertices[v];
		vertex.position = { pos.x, pos.y, pos.z };

		// normal
		if (assimpMesh->HasNormals()) {
			aiVector3D normal = normalTransform * assimpMesh->mNormals[v];
			vertex.normal = { normal.x, normal.y, normal.z };
		}

		// tangent
		if (assimpMesh->HasTangentsAndBitangents()) {
			aiVector3D tangent = normalTransform * assimpMesh->mTangents[v];
			vertex.tangentU = { tangent.x, tangent.y, tangent.z };
		}

		// UV
		if (assimpMesh->HasTextureCoords(0)) {
			vertex.texcoord.x = assimpMesh->mTextureCoords[0][v].x;
			vertex.texcoord.y = assimpMesh->mTextureCoords[0][v].y;
		}

		mesh.vertices.push_back(vertex);
	}

	// indices
	for (unsigned int f = 0; f < assimpMesh->mNumFaces; f++) {
		const aiFace& face = assimpMesh->mFaces[f];
		for (unsigned int i = 0; i < face.mNumIndices; i++) {
			mesh.indices32.push_back(BaseVertex + face.mIndices[i]);
		}
	}

	return true;
}

aiMatrix4x4 MeshLoader::GetNodeTransform(const aiNode* node) {
	aiMatrix4x4 mat;
	memcpy(&mat, &node->mTransformation, sizeof(aiMatrix4x4));
	mat.Transpose(); 
	return mat;
}

// Debug
void MeshLoader::PrintNodeHierarchy(const aiNode* node, int depth) {
	std::string indent(depth * 2, ' ');
	std::cout << indent << "Node: " << node->mName.C_Str() << std::endl;

	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		const aiMesh* mesh = m_importer.GetScene()->mMeshes[node->mMeshes[i]];
		std::cout << indent << "  Mesh: " << mesh->mName.C_Str()
			<< " (Vertices: " << mesh->mNumVertices
			<< ", Faces: " << mesh->mNumFaces << ")" << std::endl;
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		PrintNodeHierarchy(node->mChildren[i], depth + 1);
	}
}
