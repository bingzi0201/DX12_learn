#include "MeshRepository.h"

MeshRepository::MeshRepository()
{
	meshLoader = std::make_unique<MeshLoader>();
	meshLoader->Init();
}

MeshRepository& MeshRepository::Get()
{
	static MeshRepository Instance;
	return Instance;
}

void MeshRepository::Load()
{
	Mesh gunMesh;
	gunMesh.meshName = "Gun";
	meshLoader->LoadModel(L"gun.fbx", gunMesh);
	meshMap.emplace("Gun", std::move(gunMesh));

// 	Mesh personMesh;
// 	personMesh.meshName = "test";
// 	meshLoader->LoadModel(L"test.fbx", personMesh);
// 	meshMap.emplace("test", std::move(personMesh));

	Mesh boxMesh;
	boxMesh.CreateBox(1.0f, 1.0f, 1.0f, 3);
	boxMesh.meshName = "BoxMesh";
	boxMesh.GenerateBoundingBox();
	meshMap.emplace("BoxMesh", std::move(boxMesh));

	Mesh screenQuadMesh;
	screenQuadMesh.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
	screenQuadMesh.meshName = "ScreenQuadMesh";
	screenQuadMesh.GenerateBoundingBox();
	meshMap.emplace("ScreenQuadMesh", std::move(screenQuadMesh));

	Mesh sphereMesh;
	sphereMesh.CreateSphere(0.5f, 20, 20);
	sphereMesh.meshName = "SphereMesh";
	sphereMesh.GenerateBoundingBox();
	meshMap.emplace("SphereMesh", std::move(sphereMesh));

}

void MeshRepository::Unload()
{
	meshMap.clear();
}