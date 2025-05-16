#include <windows.h>
#include "Engine/Engine.h"
#include "Actor/StaticMeshActor.h"
#include "Actor/CameraActor.h"
#include "Actor/Light/DirectionalLightActor.h"
#include "Actor/Light/PointLightActor.h"
#include "Actor/Light/SpotLightActor.h"
#include "Actor/HDRSkyActor.h"

class TestWorld : public World
{
public:
	TestWorld() {}

	~TestWorld() {}

	virtual void InitWorld(Engine* inEngine) override
	{
		World::InitWorld(inEngine);

		// Add camera
		auto camera = AddActor<TCameraActor>("Camera");
		cameraComponent = camera->GetCameraComponent();
		cameraComponent->SetWorldLocation(TVector3(1.2f, -0.15f, -0.5f));
		cameraComponent->RotateY(-60.0f);
		cameraComponent->Pitch(-10.0f);
		cameraComponent->UpdateViewMatrix();

		// Add gun
		{
			auto gun = AddActor<StaticMeshActor>("Gun");
			gun->SetMesh("Gun");
			gun->SetMaterialInstance("GunInst");
			TTransform transform;
			transform.Location = TVector3(0.0f, 0.0f, 0.0f);
			transform.Rotation = TRotator(0.0f, 0.0f, 90.0f);
			transform.Scale = TVector3(1.0f, 1.0f, 1.0f);
			gun->SetActorTransform(transform);
		}


		// Add DirectionalLight
		{
			auto light = AddActor<DirectionalLightActor>("DirectionalLight");
			TTransform transform;
			transform.Location = TVector3(0.0f, 10.0f, 0.0f);
			transform.Rotation = TRotator(0.0f, 90.0f, 0.0f);
			light->SetActorTransform(transform);
			light->SetLightColor({ 1.0f, 1.0f, 1.0f });
			light->SetLightIntensity(10.0f);
		}

		// Add Enviroment Cube Light
		{
			auto HDRLight = AddActor<HDRSkyActor>("HDRSky");
			HDRLight->SetMaterialInstance("HDRSkyMatInst");
			TTransform transform;
			transform.Scale = TVector3(5000.0f, 5000.0f, 5000.0f);
			HDRLight->SetActorTransform(transform);
		}
	}
};




#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		{
			//_CrtSetBreakAlloc(550388);

			TestWorld* world = new TestWorld();
			TRenderSettings renderSettings;

			Engine engine(hInstance);
			if (!engine.Initialize(world, renderSettings))
				return 0;

			engine.Run();
			engine.Destroy();
		}

		return 0;
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}