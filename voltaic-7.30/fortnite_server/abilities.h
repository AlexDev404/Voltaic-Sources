#pragma once

namespace Abilities
{
	inline void PlayerRespawnThread(AFortPlayerControllerAthena* DeadPlayerController, FVector DeadPawnLocation)
	{
		Sleep(5000);

		auto RespawnedPawn = Core::InitializePawn(DeadPlayerController, FVector(DeadPawnLocation.X, DeadPawnLocation.Y, DeadPawnLocation.Z + 5000));

		DeadPlayerController->RespawnPlayerAfterDeath();

		auto NewCheatManager = (UFortCheatManager*)((UGameplayStatics*)UGameplayStatics::StaticClass())->STATIC_SpawnObject(UFortCheatManager::StaticClass(), DeadPlayerController);
		DeadPlayerController->CheatManager = NewCheatManager;

		NewCheatManager->RespawnPlayer();
		NewCheatManager->RespawnPlayerServer();
	}

	inline std::vector<ABuildingSMActor*> ExistingBuildings;

	inline bool CanBuild(ABuildingSMActor* BuildingActor)
	{
		for (int i = 0; i < ExistingBuildings.size(); i++)
		{
			auto Building = ExistingBuildings[i];
			if (!Building) continue;

			if (Building->K2_GetActorLocation() == BuildingActor->K2_GetActorLocation() && Building->BuildingType == BuildingActor->BuildingType)
			{
				return false;
			}
		}

		ExistingBuildings.push_back(BuildingActor);
		return true;
	}

	inline FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
	{
		auto Specs = AbilitySystemComponent->ActivatableAbilities.Items;

		for (int i = 0; i < Specs.Num(); i++)
		{
			auto& Spec = Specs[i];

			if (Spec.Handle.Handle == Handle.Handle)
			{
				return &Spec;
			}
		}

		return nullptr;
	}

	inline void InternalServerTryActiveAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey PredictionKey, FGameplayEventData* TriggerEventData)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
		if (!Spec)
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			return;
		}

		Spec->InputPressed = true;

		UGameplayAbility* InstancedAbility;
		if (!Core::InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
		{
			AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
			Spec->InputPressed = false;

			Core::MarkAbilitySpecDirty(AbilitySystemComponent, *Spec);
		}
	}


	void Initialize(UObject* Object, UFunction* Function, void* Parameters, std::string ObjectName, std::string FunctionName)
	{
		if (!Core::bStartedMatch || !Core::GetWorld()->NetDriver) return;

		if (strstr(FunctionName.c_str(), "ServerTryActivateAbility"))
		{
			auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;

			if (strstr(FunctionName.c_str(), "WithEventData"))
			{
				auto Params = (UAbilitySystemComponent_ServerTryActivateAbilityWithEventData_Params*)Parameters;
				InternalServerTryActiveAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, Params->PredictionKey, &Params->TriggerEventData);
			}
			else
			{
				auto Params = (UAbilitySystemComponent_ServerTryActivateAbility_Params*)Parameters;
				InternalServerTryActiveAbility(AbilitySystemComponent, Params->AbilityToActivate, Params->InputPressed, Params->PredictionKey, nullptr);
			}
		}
		else if (strstr(FunctionName.c_str(), "ServerAbilityRPCBatch"))
		{
			auto AbilitySystemComponent = (UAbilitySystemComponent*)Object;
			auto Params = (UAbilitySystemComponent_ServerAbilityRPCBatch_Params*)Parameters;

			InternalServerTryActiveAbility(AbilitySystemComponent, Params->BatchInfo.AbilitySpecHandle, Params->BatchInfo.InputPressed, Params->BatchInfo.PredictionKey, nullptr);
		}
		else if (strstr(FunctionName.c_str(), "ServerReviveFromDBNO"))
		{
			auto Pawn = (AFortPlayerPawn*)Object;
			auto Params = (AFortPlayerPawn_ServerReviveFromDBNO_Params*)Parameters;

			Pawn->bIsDBNO = false;
			Pawn->OnRep_IsDBNO();
			Pawn->SetHealth(Pawn->GetMaxHealth());

			((AFortPlayerControllerAthena*)Pawn->Controller)->ClientOnPawnRevived(Params->EventInstigator);
		}
		else if (strstr(FunctionName.c_str(), "ClientOnPawnDied"))
		{
			auto DeadPlayerController = (AFortPlayerControllerAthena*)Object;
			auto DeadPawn = (AFortPlayerPawnAthena*)DeadPlayerController->Pawn;
			auto DeadPawnLocation = DeadPawn->K2_GetActorLocation();
			auto DeadPlayerState = (AFortPlayerStateAthena*)DeadPlayerController->PlayerState;
			auto Params = (AFortPlayerControllerZone_ClientOnPawnDied_Params*)Parameters;

			auto KillerPawn = Params->DeathReport.KillerPawn;
			auto KillerPlayerState = (AFortPlayerStateAthena*)Params->DeathReport.KillerPlayerState;

			bool bSuicided = !KillerPawn || !KillerPlayerState || KillerPawn == DeadPawn;

			if (bSuicided)
			{
				FDeathInfo DeathInfo = { DeadPlayerState, false, EDeathCause::FallDamage };

				DeathInfo.Distance = 6900; //69 meters
				DeathInfo.DeathLocation = DeadPawnLocation;

				DeadPlayerState->ClientReportKill(DeadPlayerState);

				DeadPlayerState->DeathInfo = DeathInfo;
				DeadPlayerState->OnRep_DeathInfo();

				DeadPlayerController->ClientReceiveKillNotification(DeadPlayerState, DeadPlayerState);
			}
			else
			{
				FDeathInfo DeathInfo = { KillerPlayerState, false, EDeathCause::Banhammer };

				DeathInfo.Distance = KillerPawn->GetDistanceTo(DeadPawn);
				DeathInfo.DeathLocation = DeadPawnLocation;

				KillerPlayerState->ClientReportKill(DeadPlayerState);

				DeadPlayerState->DeathInfo = DeathInfo;
				DeadPlayerState->OnRep_DeathInfo();

				((AFortPlayerControllerAthena*)KillerPawn->Controller)->ClientReceiveKillNotification(KillerPlayerState, DeadPlayerState);

				KillerPlayerState->KillScore++;
				KillerPlayerState->TeamKillScore++;
				KillerPlayerState->OnRep_Kills();
			}

			if (DeadPlayerController->QuickBars && DeadPlayerController->WorldInventory)
			{
				auto ItemInstances = DeadPlayerController->WorldInventory->Inventory.ItemInstances;

				for (int i = 0; i < ItemInstances.Num(); i++)
				{
					auto ItemInstance = ItemInstances[i];
					if (!ItemInstance) continue;

					auto Weapon = Inventory::GetWeaponFromGuid(DeadPlayerController, ItemInstance->ItemEntry.ItemGuid);
					if (!Weapon) continue;

					ItemInstance->ItemEntry.LoadedAmmo = Weapon->GetBulletsPerClip();
				}
			}

			std::thread(PlayerRespawnThread, DeadPlayerController, DeadPawnLocation).detach();
		}
		else if (strstr(FunctionName.c_str(), "ServerHandlePickup"))
		{
			Inventory::ServerHandlePickup((AFortPlayerPawn*)Object, Parameters);
		}
		else if (strstr(FunctionName.c_str(), "ServerAttemptInventoryDrop"))
		{
			Inventory::ServerAttemptInventoryDrop((AFortPlayerControllerAthena*)Object, Parameters);
		}
		else if (strstr(FunctionName.c_str(), "ServerExecuteInventoryItem"))
		{
			Inventory::ServerExecuteInventoryItem((AFortPlayerControllerAthena*)Object, *(FGuid*)Parameters);
		}
		else if (strstr(FunctionName.c_str(), "ServerAttemptAircraftJump"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerControllerAthena_ServerAttemptAircraftJump_Params*)Parameters;

			auto Aircraft = ((AFortGameStateAthena*)Core::GetWorld()->GameState)->GetAircraft(0);
			if (!Aircraft) return;

			Core::InitializePawn(PlayerController, Aircraft->K2_GetActorLocation());

			Aircraft->PlayEffectsForPlayerJumped();
		}
		else if (strstr(FunctionName.c_str(), "OnAircraftExitedDropZone"))
		{
			auto Connections = Core::GetWorld()->NetDriver->ClientConnections;

			for (int i = 0; i < Connections.Num(); i++)
			{
				auto PlayerController = (AFortPlayerControllerAthena*)Connections[i]->PlayerController;
				if (!PlayerController || PlayerController->PlayerState->bIsSpectator)
					continue;

				if (PlayerController->IsInAircraft())
					PlayerController->ServerAttemptAircraftJump(PlayerController->GetControlRotation());
			}
		}
		else if (strstr(FunctionName.c_str(), "BuildingActor.OnDeathServer"))
		{
			auto Actor = (ABuildingActor*)Object;
			if (!Actor) return;

			for (int i = 0; i < ExistingBuildings.size(); i++)
			{
				auto Building = ExistingBuildings[i];
				if (!Building) continue;

				if (Building == Actor)
				{
					ExistingBuildings.erase(ExistingBuildings.begin() + i);
					return;
				}
			}
		}
		else if (strstr(FunctionName.c_str(), "ServerCreateBuildingActor"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerController_ServerCreateBuildingActor_Params*)Parameters;

			auto GameState = (AFortGameStateAthena*)Core::GetWorld()->GameState;

			auto BuildingActor = (ABuildingSMActor*)Core::SpawnActor(Params->BuildingClassData.BuildingClass, Params->BuildLoc, PlayerController, Math::RotToQuat(Params->BuildRot), ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (!BuildingActor) return;

			if (CanBuild(BuildingActor))
			{
				BuildingActor->DynamicBuildingPlacementType = EDynamicBuildingPlacementType::DestroyAnythingThatCollides;
				BuildingActor->SetMirrored(Params->bMirrored);
				BuildingActor->InitializeKismetSpawnedBuildingActor(BuildingActor, PlayerController);
				BuildingActor->Team = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
				return;
			}

			//BuildingActor->K2_DestroyActor();
			BuildingActor->SetActorHiddenInGame(true);
			BuildingActor->SilentDie();
		}
		else if (strstr(FunctionName.c_str(), "ServerRepairBuildingActor"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerController_ServerRepairBuildingActor_Params*)Parameters;

			if (!Params->BuildingActorToRepair) return;

			Params->BuildingActorToRepair->RepairBuilding(PlayerController, 20); //todo: not hardcode it here
		}
		else if (strstr(FunctionName.c_str(), "ServerAttemptInteract"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Params = (AFortPlayerController_ServerAttemptInteract_Params*)Parameters;
			auto ReceivingActor = Params->ReceivingActor;
			if (!ReceivingActor) return;

			if (ReceivingActor->IsA(APlayerPawn_Athena_C::StaticClass()))
			{
				((APlayerPawn_Athena_C*)ReceivingActor)->ReviveFromDBNO(PlayerController);
			}
			else if (ReceivingActor->IsA(ABuildingContainer::StaticClass())/*strstr(ReceivingActorName.c_str(), "Tiered_Chest_Athena") || strstr(ReceivingActorName.c_str(), "Tiered_Ammo_Athena")*/)
			{
				((ABuildingContainer*)ReceivingActor)->bAlreadySearched = true;
				((ABuildingContainer*)ReceivingActor)->OnRep_bAlreadySearched();


				auto ReceivingActorName = ReceivingActor->GetName();
				std::cout << "ServerAttemptInteract Container: " << ReceivingActorName << "\n";

				if (strstr(ReceivingActorName.c_str(), "Chest"))
				{
					std::cout << "Player opened a chest" << "\n";

					auto ContainerLocation = ReceivingActor->K2_GetActorLocation();

					auto Weapon = Inventory::GetRandomWeapon();
					auto Ammo = Weapon->GetAmmoWorldItemDefinition_BP();
					auto Consumable = Inventory::GetRandomConsumable();

					Core::SummonPickup(Weapon, 1, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Chest);
					Core::SummonPickup(Ammo, Ammo->DropCount, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Chest);
					Core::SummonPickup(Consumable, 1, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Chest);

					//todo: add materials
				}
				else if (strstr(ReceivingActorName.c_str(), "Ammo"))
				{
					std::cout << "Player opened an ammobox" << "\n";

					auto ContainerLocation = ReceivingActor->K2_GetActorLocation();

					auto Ammo = Inventory::GetRandomAmmo();

					Core::SummonPickup(Ammo, Ammo->DropCount, ContainerLocation, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::AmmoBox);
				}
			}

			return;
		}
		else if (strstr(FunctionName.c_str(), "ServerPlayEmoteItem"))
		{
			auto PlayerController = (AFortPlayerControllerAthena*)Object;
			auto Pawn = (APlayerPawn_Athena_C*)PlayerController->Pawn;
			auto CurrentParams = (AFortPlayerController_ServerPlayEmoteItem_Params*)Parameters;

			if (Pawn)
			{
				auto EmoteItem = CurrentParams->EmoteAsset;
				auto Animation = EmoteItem->GetAnimationHardReference(EFortCustomBodyType::All, EFortCustomGender::Both);

				if (Pawn->RepAnimMontageInfo.AnimMontage != Animation)
				{
					Pawn->RepAnimMontageInfo.AnimMontage = Animation;
					Pawn->RepAnimMontageInfo.PlayRate = 1;
					Pawn->RepAnimMontageInfo.IsStopped = false;
					Pawn->RepAnimMontageInfo.SkipPositionCorrection = true;

					Pawn->PlayAnimMontage(Animation, 1, FName());
					Pawn->OnRep_AttachmentMesh();
					Pawn->OnRep_AttachmentReplication();
					Pawn->OnRep_ReplicatedAnimMontage();
					//Pawn->Mesh->GetAnimInstance()->Montage_Play(Animation, 1, EMontagePlayReturnType::Duration, 0, true);
				}
			}
		}




		//end of thing
	}
}