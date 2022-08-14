#pragma once

namespace Core
{	
	inline UNetDriver* (*CreateNetDriver)(UEngine* Engine, UWorld* InWorld, FName NetDriverDefinition);
	inline bool (*InitListen)(UNetDriver* Driver, void* InNotify, FURL& LocalURL, bool bReuseAddressAndPort, FString& Error);
	inline void* (*SetWorld)(UNetDriver* NetDriver, UWorld* World);
	inline void (*ServerReplicateActors)(UReplicationDriver* ReplicationDriver);

	inline void (*WelcomePlayer)(UWorld* World, UNetConnection* Connection);
	inline void (*ReceiveFString)(void* Bunch, FString& Str);
	inline void (*ReceiveUniqueIdRepl)(void* Bunch, FUniqueNetIdRepl& Str);

	inline FGameplayAbilitySpecHandle* (*f_GiveAbility)(UAbilitySystemComponent* _this, FGameplayAbilitySpecHandle* outHandle, FGameplayAbilitySpec inSpec);

	inline bool (*InternalTryActivateAbility)(UAbilitySystemComponent* _this, FGameplayAbilitySpecHandle Handle, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, FGameplayEventData* TriggerEventData);
	inline void (*MarkAbilitySpecDirty)(UAbilitySystemComponent* _this, FGameplayAbilitySpec& Spec);

	inline UFortEngine* GetFortEngine()
	{
		static auto FortEngine = UObject::FindObject<UFortEngine>("FortEngine_");
		return FortEngine;
	}

	inline UWorld* GetWorld()
	{
		return GetFortEngine()->GameViewport->World;
	}

	inline FTransform GetPlayerStart(AFortPlayerControllerAthena* Controller)
	{
		TArray<AActor*> OutActors;
		((UGameplayStatics*)UGameplayStatics::StaticClass())->STATIC_GetAllActorsOfClass(GetWorld(), AFortPlayerStartWarmup::StaticClass(), &OutActors);

		auto ActorsNum = OutActors.Num();

		auto SpawnTransform = FTransform();
		SpawnTransform.Scale3D = FVector(1, 1, 1);
		SpawnTransform.Rotation = FQuat();
		SpawnTransform.Translation = FVector{ -124461, -116273, 4000 };

		auto GamePhase = ((AFortGameStateAthena*)GetWorld()->GameState)->GamePhase;

		if (ActorsNum != 0 && (GamePhase == EAthenaGamePhase::Setup || GamePhase == EAthenaGamePhase::Warmup))
		{
			auto ActorToUseNum = rand() % ActorsNum;
			auto ActorToUse = OutActors[ActorToUseNum];

			while (!ActorToUse)
			{
				ActorToUseNum = rand() % ActorsNum;
				ActorToUse = OutActors[ActorToUseNum];
			}

			SpawnTransform.Translation = ActorToUse->K2_GetActorLocation();

			Controller->WarmupPlayerStart = (AFortPlayerStartWarmup*)ActorToUse;
		}

		OutActors.FreeArray();

		return SpawnTransform;
	}

	inline AActor* SpawnActor(UClass* StaticClass, FVector Location = { 0.0f, 0.0f, 0.0f }, AActor* Owner = nullptr, FQuat Rotation = { 0, 0, 0 }, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		FTransform SpawnTransform;

		SpawnTransform.Translation = Location;
		SpawnTransform.Scale3D = FVector{ 1, 1, 1 };
		SpawnTransform.Rotation = Rotation;

		AActor* FirstActor = ((UGameplayStatics*)UGameplayStatics::StaticClass())->STATIC_BeginDeferredActorSpawnFromClass(GetWorld(), StaticClass, SpawnTransform, Flags, Owner);

		if (FirstActor)
		{
			AActor* FinalActor = ((UGameplayStatics*)UGameplayStatics::StaticClass())->STATIC_FinishSpawningActor(FirstActor, SpawnTransform);

			if (FinalActor)
				return FinalActor;
		}

		return nullptr;
	}

	template <typename ActorType>
	inline ActorType* SpawnActor(FVector Location = { 0.0f, 0.0f, 0.0f }, AActor* Owner = nullptr, FQuat Rotation = { 0, 0, 0 }, ESpawnActorCollisionHandlingMethod Flags = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		return (ActorType*)SpawnActor(ActorType::StaticClass(), Location, Owner, Rotation, Flags);
	}

	inline AFortPickup* SummonPickup(UFortItemDefinition* ItemDef, int Count, FVector Location, EFortPickupSourceTypeFlag InPickupSourceTypeFlags = EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource InPickupSpawnSource = EFortPickupSpawnSource::Chest)
	{
		auto FortPickup = SpawnActor<AFortPickup>(Location);
		if (!FortPickup) return 0;

		FortPickup->bReplicates = true;

		FortPickup->PrimaryPickupItemEntry.Count = Count;
		FortPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDef;

		FortPickup->OnRep_PrimaryPickupItemEntry();
		FortPickup->TossPickup(Location, 0, 6, true, InPickupSourceTypeFlags, InPickupSpawnSource);
		//FortPickup->TossPickup(Location, Pawn, 6, true);

		return FortPickup;
	}

	inline void GiveAbility(UFortAbilitySystemComponent* AbilitySystemComponent, UObject* GameplayAbility)
	{
		for (int i = 0; i < AbilitySystemComponent->ActivatableAbilities.Items.Num(); i++)
			if (AbilitySystemComponent->ActivatableAbilities.Items[i].Ability == GameplayAbility)
				return;

		FGameplayAbilitySpecHandle outHandle;
		f_GiveAbility(AbilitySystemComponent, &outHandle, { -1, -1, -1, { rand() }, (UGameplayAbility*)GameplayAbility, 1, -1 });
	}

#include "inventory.h"

	inline bool bStartedMatch = false;

	inline AFortPlayerPawnAthena* InitializePawn(AFortPlayerControllerAthena* PlayerController, FVector Location)
	{
		if (PlayerController->Pawn)
			PlayerController->Pawn->K2_DestroyActor();

		auto Pawn = SpawnActor<APlayerPawn_Athena_C>(Location, PlayerController);

		Pawn->Owner = PlayerController;
		Pawn->OnRep_Owner();

		Pawn->Controller = PlayerController;
		Pawn->OnRep_Controller();

		Pawn->PlayerState = PlayerController->PlayerState;
		Pawn->OnRep_PlayerState();

		Pawn->bAlwaysRelevant = true;

		PlayerController->Pawn = Pawn;
		PlayerController->AcknowledgedPawn = Pawn;
		PlayerController->Character = Pawn;
		PlayerController->OnRep_Pawn();
		PlayerController->Possess(Pawn);

		Pawn->SetMaxHealth(100);
		Pawn->SetMaxShield(100);

		Pawn->SetHealth(100);
		Pawn->SetShield(100);

		static auto DeathAbility = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_Death.GA_DefaultPlayer_Death_C");
		static auto InteractUseAbility = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_InteractUse.GA_DefaultPlayer_InteractUse_C");
		static auto InteractSearchAbility = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_InteractSearch.GA_DefaultPlayer_InteractSearch_C");
		static auto EmoteAbility = UObject::FindClass("BlueprintGeneratedClass GAB_Emote_Generic.GAB_Emote_Generic_C");
		static auto TrapBuildAbility = UObject::FindClass("BlueprintGeneratedClass GA_TrapBuildGeneric.GA_TrapBuildGeneric_C");
		static auto DanceGrenadeAbility = UObject::FindClass("BlueprintGeneratedClass GA_DanceGrenade_Stun.GA_DanceGrenade_Stun_C");
		static auto VehicleEnter = UObject::FindClass("BlueprintGeneratedClass GA_AthenaEnterVehicle.GA_AthenaEnterVehicle_C");
		static auto VehicleExit = UObject::FindClass("BlueprintGeneratedClass GA_AthenaExitVehicle.GA_AthenaExitVehicle_C");
		static auto InVehicle = UObject::FindClass("BlueprintGeneratedClass GA_AthenaInVehicle.GA_AthenaInVehicle_C");

		GiveAbility(Pawn->AbilitySystemComponent, UFortGameplayAbility_Sprint::StaticClass()->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, UFortGameplayAbility_Reload::StaticClass()->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, UFortGameplayAbility_RangedWeapon::StaticClass()->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, UFortGameplayAbility_Jump::StaticClass()->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, DeathAbility->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, InteractUseAbility->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, InteractSearchAbility->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, EmoteAbility->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, TrapBuildAbility->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, DanceGrenadeAbility->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, VehicleEnter->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, VehicleExit->CreateDefaultObject());
		GiveAbility(Pawn->AbilitySystemComponent, InVehicle->CreateDefaultObject());

		return Pawn;
	}

#include "abilities.h"


	inline void hk_ProcessEvent(UObject* Object, UFunction* Function, void* Parameters)
	{
		if (Object && Function)
		{
			auto ObjectName = Object->GetFullName();
			auto FunctionName = Function->GetFullName();

			if (!bStartedMatch && strstr(FunctionName.c_str(), "ReadyToStartMatch"))
			{
				bStartedMatch = true;
				std::cout << "READY TO STARTMATCH\n";

				auto GameState = (AFortGameStateAthena*)GetWorld()->GameState;
				auto GameMode = (AFortGameModeAthena*)GetWorld()->AuthorityGameMode;

				GameState->bGameModeWillSkipAircraft = true;
				GameState->AircraftStartTime = 9999.9f;
				GameState->WarmupCountdownEndTime = 99999.9f;

				GameState->GamePhase = EAthenaGamePhase::Warmup;
				GameState->OnRep_GamePhase(EAthenaGamePhase::None);

				GameMode->bDisableGCOnServerDuringMatch = true;

				GameMode->bEnableReplicationGraph = true;

				auto Playlist = UObject::FindObjectFast<UFortPlaylistAthena>("/Game/Athena/Playlists/Playground/Playlist_Playground.Playlist_Playground");
				if(!Playlist) return o_ProcessEvent(Object, Function, Parameters);

				GameState->CurrentPlaylistId = Playlist->PlaylistId;
				GameState->OnRep_CurrentPlaylistId();

				GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
				GameState->OnRep_CurrentPlaylistInfo();

				GameMode->StartPlay();
				GameMode->StartMatch();


				auto NewNetDriver = CreateNetDriver(GetFortEngine(), GetWorld(), FName(282));

				NewNetDriver->NetDriverName = FName(282);
				NewNetDriver->World = GetWorld();

				FString Error;
				auto InURL = FURL();
				InURL.Port = 7777;

				InitListen(NewNetDriver, GetWorld(), InURL, true, Error);

				SetWorld(NewNetDriver, GetWorld());

				ServerReplicateActors = decltype(ServerReplicateActors)(NewNetDriver->ReplicationDriver->Vtable[0x56]);

				GetWorld()->NetDriver = NewNetDriver;
				GetWorld()->LevelCollections[0].NetDriver = NewNetDriver;
				GetWorld()->LevelCollections[1].NetDriver = NewNetDriver;

				((AFortGameModeAthena*)GetWorld()->AuthorityGameMode)->GameSession->MaxPlayers = 100;
				//((AFortGameModeAthena*)GetWorld()->AuthorityGameMode)->NumPlayers = 1;
				//GameState->PlayersLeft = 3;
				//GameState->TotalPlayers = 3;


				std::cout << "Server is listening!\n";
			}
			

			if (strstr(FunctionName.c_str(), "ServerAcknowledgePossession"))
			{
				auto PlayerController = (APlayerController*)Object;
				auto Params = (APlayerController_ServerAcknowledgePossession_Params*)Parameters;

				PlayerController->AcknowledgedPawn = PlayerController->Pawn;

				return;
			}
			
			Abilities::Initialize(Object, Function, Parameters, ObjectName, FunctionName);
		}

		return o_ProcessEvent(Object, Function, Parameters);
	}

	uintptr_t (*o_GetNetMode)(UWorld* World);
	uintptr_t hk_GetNetMode(UWorld* World)
	{
		return ENetMode::NM_ListenServer;
	}

	inline void (*o_TickFlush)(UNetDriver* Driver, float DeltaSeconds);
	inline void hk_TickFlush(UNetDriver* Driver, float DeltaSeconds)
	{
		auto NetDriver = GetWorld()->NetDriver;

		if (NetDriver && NetDriver->ClientConnections.Num() && !NetDriver->ClientConnections[0]->InternalAck)
			if (auto ReplicationDriver = NetDriver->ReplicationDriver)
				ServerReplicateActors(ReplicationDriver);

		if (GetAsyncKeyState(VK_F5) & 1)
		{
			for (int i = 0; i < NetDriver->ClientConnections.Num(); i++)
			{
				auto PlayerController = (AFortPlayerControllerAthena*)NetDriver->ClientConnections[i]->PlayerController;
				if (!PlayerController || PlayerController->PlayerState->bIsSpectator)
					continue;

				std::cout << "AmmoCount: " << ((AFortPlayerPawnAthena*)PlayerController->Pawn)->CurrentWeapon->AmmoCount << "\n";
			}
		}

		if (GetAsyncKeyState(VK_F6) & 1)
		{
			auto GameState = (AFortGameStateAthena*)GetWorld()->GameState;
			GameState->bGameModeWillSkipAircraft = false;
			GameState->AircraftStartTime = 0;
			GameState->WarmupCountdownEndTime = 0;

			((UKismetSystemLibrary*)UKismetSystemLibrary::StaticClass())->STATIC_ExecuteConsoleCommand(GetWorld(), L"startaircraft", nullptr);
		}

		return o_TickFlush(NetDriver, DeltaSeconds);
	}

	inline void (*o_NotifyControlMessage)(UWorld* World, UNetConnection* Connection, uint8_t MessageType, void* Bunch);
	inline void hk_NotifyControlMessage(UWorld* World, UNetConnection* Connection, uint8_t MessageType, int64* Bunch)
	{
		std::cout << "NotifyControlMessage: " << std::to_string(MessageType) << '\n';

		if (MessageType == 4) //NMT_Netspeed
		{
			Connection->CurrentNetSpeed = 30000;
		}
		else if (MessageType == 5) //NMT_Login
		{
			Bunch[7] += (16 * 1024 * 1024);

			auto OnlinePlatformName = FString(L"");

			ReceiveFString(Bunch, Connection->ClientResponse);
			ReceiveFString(Bunch, Connection->RequestURL);
			ReceiveUniqueIdRepl(Bunch, Connection->PlayerID);
			ReceiveFString(Bunch, OnlinePlatformName);

			Bunch[7] -= (16 * 1024 * 1024);

			WelcomePlayer(GetWorld(), Connection);
		}
		else
			o_NotifyControlMessage(GetWorld(), Connection, MessageType, (void*)Bunch);
	}

	inline int teamidx = 4;

	inline APlayerController* (*o_SpawnPlayActor)(UWorld* World, UPlayer* NewPlayer, ENetRole RemoteRole, FURL& URL, FUniqueNetIdRepl UniqueId, SDK::FString& Error, uint8_t NetPlayerIndex);
	inline APlayerController* hk_SpawnPlayActor(UWorld* World, UPlayer* NewPlayer, ENetRole RemoteRole, FURL& URL, FUniqueNetIdRepl UniqueId, SDK::FString& Error, uint8_t NetPlayerIndex)
	{
		auto PlayerController = (AFortPlayerControllerAthena*)o_SpawnPlayActor(GetWorld(), NewPlayer, RemoteRole, URL, UniqueId, Error, NetPlayerIndex);

		NewPlayer->PlayerController = PlayerController;
		NewPlayer->CurrentNetSpeed = 30000;

		auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
		std::wcout << L"Spawning Player: " << PlayerState->GetPlayerName().c_str() << L"\n";

		auto Pawn = InitializePawn(PlayerController, GetPlayerStart(PlayerController).Translation);

		PlayerController->bHasClientFinishedLoading = true;
		PlayerController->bHasServerFinishedLoading = true;
		PlayerController->bHasInitiallySpawned = true;
		PlayerController->OnRep_bHasServerFinishedLoading();

		PlayerState->bHasFinishedLoading = true;
		PlayerState->bHasStartedPlaying = true;
		PlayerState->OnRep_bHasStartedPlaying();


		//static auto HeroType = UObject::FindObjectFast<UFortHeroType>("/Game/Heroes/Outlander/ItemDefinition/HID_Outlander_015_F_V1_SR_T04.HID_Outlander_015_F_V1_SR_T04");

		//PlayerState->HeroType = HeroType;
		//PlayerState->OnRep_HeroType();

		static auto Head = UObject::FindObjectFast<UCustomCharacterPart>("/Game/Characters/CharacterParts/Female/Medium/Heads/F_Med_Head1.F_Med_Head1");
		static auto Body = UObject::FindObjectFast<UCustomCharacterPart>("/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01");

		PlayerState->CharacterParts.Parts[(uint8_t)EFortCustomPartType::Head] = Head;
		PlayerState->CharacterParts.Parts[(uint8_t)EFortCustomPartType::Body] = Body;

		//PlayerState->OnRep_CharacterParts();


		Inventory::Initialize(PlayerController);


		PlayerState->TeamIndex = EFortTeam(teamidx);

		PlayerState->PlayerTeam->TeamMembers.Add(PlayerController);
		PlayerState->PlayerTeam->Team = EFortTeam(teamidx);

		PlayerState->SquadId = teamidx - 1;
		PlayerState->OnRep_PlayerTeam();
		PlayerState->OnRep_SquadId();

		teamidx++;

		PlayerController->OverriddenBackpackSize = 100;

		std::cout << "Spawned PlayerController: " << PlayerController << "\n";

		return PlayerController;
	}

	bool (*o_LocalPlayerSpawnPlayActor)(ULocalPlayer* Player, const FString& URL, FString& OutError, UWorld* World);
	bool hk_LocalPlayerSpawnPlayActor(ULocalPlayer* Player, const FString& URL, FString& OutError, UWorld* World)
	{
		if (Player != GetWorld()->OwningGameInstance->LocalPlayers[0])
			return o_LocalPlayerSpawnPlayActor(Player, URL, OutError, World);
	}

	void (*o_OnReload)(AFortWeapon* _this, unsigned int a2);
	void hk_OnReload(AFortWeapon* _this, unsigned int a2)
	{
		o_OnReload(_this, a2);
		/*
		auto PlayerController = ((APawn*)_this->Owner)->Controller;
		if (!PlayerController) return;

		int AmmoToRemove = a2;

		//o_OnReload(_this, a2);

		if(_this->AmmoCount < _this->GetBulletsPerClip())
			_this->AmmoCount += AmmoToRemove;
		*/
	}


	bool (*o_CanActivateAbility)(UGameplayAbility* GameplayAbility, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags);
	bool hk_CanActivateAbility(UGameplayAbility* GameplayAbility, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags)
	{
		o_CanActivateAbility(GameplayAbility, Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

		return true;
	}



	inline void Initialize()
	{
		CreateNetDriver = decltype(CreateNetDriver)(Memory::FindPattern("48 89 5C 24 10 57 48 83 EC ? 48 8B 81 D0 0B 00 00"));
		InitListen = decltype(InitListen)(Memory::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 50 48 8B BC 24 ? ? ? ? 49 8B F0"));
		SetWorld = decltype(SetWorld)(Memory::FindPattern("48 89 5C 24 08 57 48 83 EC ? 48 8B FA 48 8B D9 48 8B 91 40 01 00 00"));


		WelcomePlayer = decltype(WelcomePlayer)(Memory::FindPattern("48 8B C4 55 48 8D A8 48 FF FF FF"));
		ReceiveFString = decltype(ReceiveFString)(Memory::FindPattern("48 89 5C 24 18 55 56 57 41 56 41 57 48 8D 6C 24 C9 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 27 0F B6 41 28"));
		ReceiveUniqueIdRepl = decltype(ReceiveUniqueIdRepl)(Memory::FindPattern("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 40 F6 41 28 40 48 8B FA 48 8B D9 0F 84 ? ? ? ? F6 41 2B 02"));


		f_GiveAbility = decltype(f_GiveAbility)(Memory::FindPattern("48 89 5C 24 10 48 89 6C 24 18 48 89 7C 24 20 41 56 48 83 EC ? 83 B9 60 05"));

		InternalTryActivateAbility = decltype(InternalTryActivateAbility)(Memory::FindPattern("4C 89 4C 24 20 4C 89 44 24 18 89 54 24 10 55 53 56 57 41 54"));
		MarkAbilitySpecDirty = decltype(MarkAbilitySpecDirty)(Memory::FindPattern("48 89 5C 24 18 48 89 7C 24 20 41 56 48 83 EC ? 48 8B 01 41 0F B6 D8"));


		auto NetDebug = Memory::FindPattern("40 55 56 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B 01 48 8B F1 FF 90 ? ? ? ? 4C 8B F0 48 85 C0 0F");
		byte NetDebugPatch[] = { 0xC3, 0x90 };
		for (int i = 0; i < 2; i++)
			*(BYTE*)(NetDebug + i) = NetDebugPatch[i];

		auto KickPlayer = Memory::FindPattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 49 8B F0 48 8B DA 48 85 D2");
		byte KickPlayerPatch[] = { 0xC3, 0x90, 0x90, 0x90, 0x90 };
		for (int i = 0; i < 5; i++)
			*(BYTE*)(KickPlayer + i) = KickPlayerPatch[i];


		auto GetNetMode = Memory::FindPattern("48 89 5C 24 08 57 48 83 EC ? 48 8B 01 48 8B D9 FF 90 40 01 00 00 4C 8B 83 10 01 00 00");
		MH_CreateHook((void*)GetNetMode, hk_GetNetMode, (void**)&o_GetNetMode);
		MH_EnableHook((void*)GetNetMode);

		auto TickFlush = Memory::FindPattern("4C 8B DC 55 49 8D AB 78 FE FF FF 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 00 01 00 00 49 89 5B 18");
		MH_CreateHook((void*)TickFlush, hk_TickFlush, (void**)&o_TickFlush);
		MH_EnableHook((void*)TickFlush);

		auto LocalPlayerSpawnPlayActor = Memory::FindPattern("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55 41 56 41 57 48 8D 6C 24 F0 48 81 EC ? ? ? ? 48 8B D9");
		MH_CreateHook((void*)LocalPlayerSpawnPlayActor, hk_LocalPlayerSpawnPlayActor, (void**)&o_LocalPlayerSpawnPlayActor);
		MH_EnableHook((void*)LocalPlayerSpawnPlayActor);

		auto NotifyControlMessage = Memory::FindPattern("48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D AC 24 D0 F9 FF FF");
		MH_CreateHook((void*)NotifyControlMessage, hk_NotifyControlMessage, (void**)&o_NotifyControlMessage);
		MH_EnableHook((void*)NotifyControlMessage);

		auto SpawnPlayActor = Memory::FindPattern("48 8B C4 4C 89 48 20 44 89 40 18 48 89 50 10 48 89 48 08 55 56");
		MH_CreateHook((void*)SpawnPlayActor, hk_SpawnPlayActor, (void**)&o_SpawnPlayActor);
		MH_EnableHook((void*)SpawnPlayActor);

		auto OnReload = Memory::FindPattern("89 54 24 10 55 41 56 48 8D 6C 24 B1 48 81 EC ? ? ? ? 80 B9 18 01");
		MH_CreateHook((void*)OnReload, hk_OnReload, (void**)&o_OnReload);
		MH_EnableHook((void*)OnReload);

		auto CanActivateAbility = Memory::FindPattern("4C 89 4C 24 20 55 56 57 41 56 48 8D 6C 24 D1");
		MH_CreateHook((void*)CanActivateAbility, hk_CanActivateAbility, (void**)&o_CanActivateAbility);
		MH_EnableHook((void*)CanActivateAbility);

		auto ProcessEvent = GetFortEngine()->Vtable[0x40];
		MH_CreateHook((void*)ProcessEvent, hk_ProcessEvent, (void**)&o_ProcessEvent);
		MH_EnableHook((void*)ProcessEvent);
	}
}