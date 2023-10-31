// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityZoneGraphSpawnPointsGenerator.h"

#include "MassOrderedZGSpawnGenerator.generated.h"

UCLASS()
class UMassOrderedZGSpawnGenerator : public UMassEntityZoneGraphSpawnPointsGenerator
{
	GENERATED_BODY()

public:
	virtual void Generate(UObject& QueryOwner, TConstArrayView<FMassSpawnedEntityType> EntityTypes, int32 Count, FFinishedGeneratingSpawnDataSignature& FinishedGeneratingSpawnPointsDelegate) const override;

};
