#include "Actor.h"

Actor::Actor(const std::string& name)
{
	SetName(name);
}

void Actor::SetActorTransform(const TTransform& newTransform)
{
	rootComponent->SetWorldTransform(newTransform);
}

TTransform Actor::GetActorTransform() const 
{
	return rootComponent->GetWorldTransform();
}

void Actor::SetActorLocation(const TVector3& newLocation)
{
	rootComponent->SetWorldLocation(newLocation);
}

TVector3 Actor::GetActorLocation() const
{
	return rootComponent->GetWorldLocation();
}

void Actor::SetActorRotation(const TRotator& newRotation)
{
	rootComponent->SetWorldRotation(newRotation);
}

TRotator Actor::GetActorRotation() const
{
	return rootComponent->GetWorldRotation();
}

void Actor::SetActorPrevTransform(const TTransform& prevTransform)
{
	rootComponent->SetPrevWorldTransform(prevTransform);
}

TTransform Actor::GetActorPrevTransform() const
{
	return rootComponent->GetPrevWorldTransform();
}