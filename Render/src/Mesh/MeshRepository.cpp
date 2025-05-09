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

	Mesh BoxMesh;
	BoxMesh.CreateBox(1.0f, 1.0f, 1.0f, 3);
	BoxMesh.meshName = "BoxMesh";
	BoxMesh.GenerateBoundingBox();
	meshMap.emplace("BoxMesh", std::move(BoxMesh));

// 	Mesh QuadMesh;
// 	QuadMesh.CreateQuad(-0.5f, 0.5f, 1.0f, 1.0f, 0.0f);
// 	QuadMesh.meshName = "QuadMesh";
// 	QuadMesh.GenerateBoundingBox();
// 	meshMap.emplace("QuadMesh", std::move(QuadMesh));

	Mesh ScreenQuadMesh;
	ScreenQuadMesh.CreateQuad(-1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
	ScreenQuadMesh.meshName = "ScreenQuadMesh";
	ScreenQuadMesh.GenerateBoundingBox();
	meshMap.emplace("ScreenQuadMesh", std::move(ScreenQuadMesh));
}

void MeshRepository::Unload()
{
	meshMap.clear();
}