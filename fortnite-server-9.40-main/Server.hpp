#pragma once
// I lik monkies
#include "Finder.hpp"

using namespace Unreal;

uintptr_t(*o_GetNetMode)(UObject* World);
uintptr_t hk_GetNetMode(UObject* World)
{
	return ENetMode::NM_ListenServer;
}

inline UObject* InitializePawn(UObject* PlayerController, FVector Location)
{
	if (*Finder::Find(PlayerController, "Pawn"))
		*Finder::Find(*Finder::Find(PlayerController, "Pawn"), "K2_DestroyActor");

	auto Pawn = SpawnActor(Game::GWorld, FindObject("PlayerPawn_Athena_C"), new FVector(Location), new FVector{}, {});

	*Finder::Find(Pawn, "Owner") = PlayerController;
	Pawn->ProcessEvent(FindObject("OnRep_Owner"));

	*Finder::Find(Pawn, "Controller") = PlayerController;
	Pawn->ProcessEvent(FindObject("OnRep_Controller"));

	*Finder::Find(Pawn, "PlayerState") = *Finder::Find(PlayerController, "PlayerState");
	Pawn->ProcessEvent(FindObject("OnRep_PlayerState"));

	*Finder::Find<bool*>(Pawn, "bAlwaysRelevant") = true;

	*Finder::Find(PlayerController, "Pawn") = Pawn;
	*Finder::Find(PlayerController, "AcknowledgedPawn") = Pawn;
	*Finder::Find(PlayerController, "Character") = Pawn;
	PlayerController->ProcessEvent(FindObject("OnRep_Pawn"));
	PlayerController->ProcessEvent(FindObject("Possess"), &Pawn);

	//Pawn->SetMaxHealth(100);
	//Pawn->SetMaxShield(100);

	//Pawn->SetHealth(100);
	//Pawn->SetShield(100);

	/*static auto DeathAbility = UObject::FindClass("BlueprintGeneratedClass GA_DefaultPlayer_Death.GA_DefaultPlayer_Death_C");
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
	GiveAbility(Pawn->AbilitySystemComponent, InVehicle->CreateDefaultObject());*/

	return Pawn;
}

inline void (*o_TickFlush)(UObject* Driver, float DeltaSeconds);
inline void hk_TickFlush(UObject* Driver, float DeltaSeconds)
{
	auto NetDriver = *Finder::Find(Game::GWorld, "NetDriver");

	if (NetDriver && NetDriver->ClientConnections.Num() && !NetDriver->ClientConnections[0]->InternalAck)
		if (auto ReplicationDriver = NetDriver->ReplicationDriver)
			ServerReplicateActors(ReplicationDriver);

	return o_TickFlush(NetDriver, DeltaSeconds);
}


inline void (*o_NotifyControlMessage)(UObject* World, UObject* Connection, uint8_t MessageType, void* Bunch);
inline void hk_NotifyControlMessage(UObject* World, UObject* Connection, uint8_t MessageType, __int64* Bunch)
{
	std::cout << "NotifyControlMessage: " << std::to_string(MessageType) << '\n';

	if (MessageType == 4) //NMT_Netspeed
	{
		*Finder::Find<int*>(Connection, "CurrentNetSpeed") = 30000;
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

		WelcomePlayer(Game::GWorld, Connection);
	}
	else
		o_NotifyControlMessage(Game::GWorld, Connection, MessageType, (void*)Bunch);
}

inline int teamidx = 4;

inline UObject* (*o_SpawnPlayActor)(UObject* World, UObject* NewPlayer, ENetRole RemoteRole, FURL& URL, void* UniqueId, FString& Error, uint8_t NetPlayerIndex);
inline UObject* hk_SpawnPlayActor(UObject* World, UObject* NewPlayer, ENetRole RemoteRole, FURL& URL, void* UniqueId, FString& Error, uint8_t NetPlayerIndex)
{
	auto PlayerController = (UObject*)o_SpawnPlayActor(Game::GWorld, NewPlayer, RemoteRole, URL, UniqueId, Error, NetPlayerIndex);

	*Finder::Find(NewPlayer, "PlayerController") = PlayerController;
	*Finder::Find<int*>(NewPlayer, "CurrentNetSpeed") = 30000;

	auto PlayerState = (UObject*)Finder::Find(PlayerController, "PlayerState");
	//std::wcout << L"Spawning Player: " << PlayerState->GetPlayerName().c_str() << L"\n";

	auto Pawn = InitializePawn(PlayerController, GetPlayerStart(PlayerController).Translation);


	*Finder::Find<bool*>(PlayerController, "bHasClientFinishedLoading") = true;
	*Finder::Find<bool*>(PlayerController, "bHasServerFinishedLoading") = true;
	*Finder::Find<bool*>(PlayerController, "bHasInitiallySpawned") = true;
	PlayerController->ProcessEvent(FindObject("OnRep_bHasServerFinishedLoading"));

	*Finder::Find<bool*>(PlayerState, "bHasFinishedLoading") = true;
	*Finder::Find<bool*>(PlayerState, "bHasStartedPlaying") = true;
	PlayerState->ProcessEvent(FindObject("OnRep_bHasStartedPlaying"));

	PlayerState->ProcessEvent(FindObject("ServerChoosePart"));

	static auto Head = FindObject("/Game/Characters/CharacterParts/Female/Medium/Heads/F_Med_Head1.F_Med_Head1");
	static auto Body = FindObject("/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01");

	PlayerState->CharacterParts.Parts[(uint8_t)EFortCustomPartType::Head] = Head;
	PlayerState->CharacterParts.Parts[(uint8_t)EFortCustomPartType::Body] = Body;


	//Inventory::Initialize(PlayerController);

	PlayerState->ProcessEvent(FindObject("OnRep_PlayerTeam"));
	PlayerState->ProcessEvent(FindObject("OnRep_SquadId"));

	std::cout << "Spawned PlayerController: " << PlayerController << "\n";

	return PlayerController;
}

bool (*o_LocalPlayerSpawnPlayActor)(UObject* Player, const FString& URL, FString& OutError, UObject* World);
bool hk_LocalPlayerSpawnPlayActor(UObject* Player, const FString& URL, FString& OutError, UObject* World)
{
	if (Player != Finder::Find<TArray<UObject*>*>(*Finder::Find(Game::GWorld, "OwningGameInstance"), "LocalPlayers")->Data[0]);
		return o_LocalPlayerSpawnPlayActor(Player, URL, OutError, World);
}

//void (*o_OnReload)(UObject* _this, unsigned int a2);
//void hk_OnReload(UObject* _this, unsigned int a2)
//{
//	o_OnReload(_this, a2);
//}