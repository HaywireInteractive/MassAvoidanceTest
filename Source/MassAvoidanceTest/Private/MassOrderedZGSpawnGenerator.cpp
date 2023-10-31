// Fill out your copyright notice in the Description page of Project Settings.

#include "MassOrderedZGSpawnGenerator.h"

#include "ZoneGraphSubsystem.h"
#include "MassSpawnLocationProcessor.h"
#include "MassGameplaySettings.h"

void UMassOrderedZGSpawnGenerator::Generate(UObject& QueryOwner, TConstArrayView<FMassSpawnedEntityType> EntityTypes, int32 Count, FFinishedGeneratingSpawnDataSignature& FinishedGeneratingSpawnPointsDelegate) const
{
	if (Count <= 0)
	{
		FinishedGeneratingSpawnPointsDelegate.Execute(TArray<FMassEntitySpawnDataGeneratorResult>());
		return;
	}
	
	const UZoneGraphSubsystem* ZoneGraph = UWorld::GetSubsystem<UZoneGraphSubsystem>(QueryOwner.GetWorld());
	if (ZoneGraph == nullptr)
	{
		UE_VLOG_UELOG(&QueryOwner, LogMass, Error, TEXT("No zone graph subsystem found in world"));
		return;
	}

	TArray<FVector> Locations;
	
	const FRandomStream RandomStream(GetRandomSelectionSeed());
	const TConstArrayView<FRegisteredZoneGraphData> RegisteredZoneGraphs = ZoneGraph->GetRegisteredZoneGraphData();
	if (RegisteredZoneGraphs.IsEmpty())
	{
		UE_VLOG_UELOG(&QueryOwner, LogMass, Error, TEXT("No zone graphs found"));
		return;
	}

	for (const FRegisteredZoneGraphData& Registered : RegisteredZoneGraphs)
	{
		if (Registered.bInUse && Registered.ZoneGraphData)
		{
			GeneratePointsForZoneGraphData(*Registered.ZoneGraphData, Locations, RandomStream);
		}
	}

	if (Locations.IsEmpty())
	{
		UE_VLOG_UELOG(&QueryOwner, LogMass, Error, TEXT("No locations found on zone graphs"));
		return;
	}

	// If we generated too many, shrink it.
	if (Locations.Num() > Count)
	{
		Locations.SetNum(Count);
	}

	// Build array of entity types to spawn.
	TArray<FMassEntitySpawnDataGeneratorResult> Results;
	BuildResultsFromEntityTypes(Count, EntityTypes, Results);

	const int32 LocationCount = Locations.Num();
	int32 LocationIndex = 0;

	// Distribute points amongst the entities to spawn.
	for (FMassEntitySpawnDataGeneratorResult& Result : Results)
	{
		// @todo: Make separate processors and pass the ZoneGraph locations directly.
		Result.SpawnDataProcessor = UMassSpawnLocationProcessor::StaticClass();
		Result.SpawnData.InitializeAs<FMassTransformsSpawnData>();
		FMassTransformsSpawnData& Transforms = Result.SpawnData.GetMutable<FMassTransformsSpawnData>();

		Transforms.Transforms.Reserve(Result.NumEntities);
		for (int i = 0; i < Result.NumEntities; i++)
		{
			FTransform& Transform = Transforms.Transforms.AddDefaulted_GetRef();
			Transform.SetLocation(Locations[LocationIndex % LocationCount]);
			LocationIndex++;
		}
	}

#if ENABLE_VISUAL_LOG
	UE_VLOG(this, LogMass, Log, TEXT("Spawning at %d locations"), LocationIndex);
	if (GetDefault<UMassGameplaySettings>()->bLogSpawnLocations)
	{
		if (FVisualLogEntry* LogEntry = FVisualLogger::Get().GetLastEntryForObject(this))
		{
			FVisualLogShapeElement Element(TEXT(""), FColor::Orange, /*Thickness*/20, LogMass.GetCategoryName());

			Element.Points.Reserve(LocationIndex);
			for (const FMassEntitySpawnDataGeneratorResult& Result : Results)
			{
				const FMassTransformsSpawnData& Transforms = Result.SpawnData.Get<FMassTransformsSpawnData>();
				for (int i = 0; i < Result.NumEntities; i++)
				{
					Element.Points.Add(Transforms.Transforms[i].GetLocation());
				}
			}
			
			Element.Type = EVisualLoggerShapeElement::SinglePoint;
			Element.Verbosity = ELogVerbosity::Display;
			LogEntry->AddElement(Element);
		}
	}
#endif // ENABLE_VISUAL_LOG

	FinishedGeneratingSpawnPointsDelegate.Execute(Results);	
}
