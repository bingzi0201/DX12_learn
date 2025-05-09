#pragma once
#include <vector>
#include <memory>
#include <string>
#include "../Component/Component.h"
#include "../Math/Transform.h"

class Actor
{
public:
	Actor(const std::string& name);
	virtual ~Actor() {}

protected:
	std::string actorName;
	std::vector <std::unique_ptr<Component>> components;
	Component* rootComponent = nullptr;

public:
	virtual void Tick(float DeltaSeconds) {}

public:
	template<typename T>
	T* AddComponent()
	{
		auto newComponent = std::make_unique<T>();
		T* result = newComponent.get();
		components.push_back(std::move(newComponent));
		return result;
	}

	std::vector<Component*> GetComponents()
	{
		std::vector<Component*> result;
		for (const auto& component : components)
		{
			result.push_back(component.get());
		}
		return result;
	}

	template<typename T>
	std::vector<T*> GetComponentsOfClass()
	{
		std::vector<T*> result;
		for (const auto& component : components)
		{
			T* componentOfClass = dynamic_cast<T*>(component.get());
			if (componentOfClass)
			{
				result.push_back(componentOfClass);
			}
		}
		return result;
	}

	Component* GetRootComponent() const 
	{
		return rootComponent;
	}

	virtual void SetActorTransform(const TTransform& newTransform);
	TTransform GetActorTransform() const;

	void SetActorLocation(const TVector3& newLocation);
	TVector3 GetActorLocation() const;

	void SetActorRotation(const TRotator& newRotation);
	TRotator GetActorRotation() const;

	void SetActorPrevTransform(const TTransform& prevTransform);
	TTransform GetActorPrevTransform() const;

	void SetName(std::string name) { actorName = name; }
	std::string GetName() const { return actorName; }
};