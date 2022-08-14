#pragma once
// credits to radium
#pragma once
float GVersion = 0.0f; //(TODO) Get the version somehow?
#include "Sigs.hpp"
#include <string>
#include <locale>
#include <iostream>
#include <ostream>
#include <fstream>
#include <sstream>
namespace Unreal {
	template<class T>
	struct TArray
	{
		friend class FString;

	public:
		inline TArray()
		{
			Data = nullptr;
			Count = Max = 0;
		};

		inline int Num() const
		{
			return Count;
		};

		inline void Add(T InputData)
		{
			Data = (T*)realloc(Data, sizeof(T) * (Count + 1));
			Data[Count++] = InputData;
			Max = Count;
		};

		inline void Remove(int32_t Index)
		{
			TArray<T> NewArray;
			for (int i = 0; i < this->Count; i++)
			{
				if (i == Index)
					continue;

				NewArray.Add(this->Data[i]);
			}
			this->Data = (T*)realloc(NewArray.Data, sizeof(T) * (NewArray.Count));
			this->Count = NewArray.Count;
			this->Max = NewArray.Count;
		}

		T* Data;
		int Count;
		int Max;
	};


	struct FString : private TArray<wchar_t>
	{
		FString()
		{
		};

		FString(const wchar_t* other)
		{
			Max = Count = *other ? std::wcslen(other) + 1 : 0;

			if (Count)
			{
				Data = const_cast<wchar_t*>(other);
			}
		}

		bool IsValid() const
		{
			return Data != nullptr;
		}

		const wchar_t* c_str() const
		{
			return Data;
		}

		std::string ToString() const
		{
			auto length = std::wcslen(Data);

			std::string str(length, '\0');

			std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);

			return str;
		}
	};

	struct UObject;
	struct FName;
	//New
	FString(__fastcall* GetObjectFullName)(UObject* In);

	//Old
	void(__fastcall* FNameToString)(FName* NameIn, FString& Out);
	void(__fastcall* FnFree)(__int64);

	struct FName
	{
		uint32_t ComparisonIndex;
		uint32_t DisplayIndex;

		FName() = default;

		std::string GetName() {
			FString temp;
			FNameToString(this, temp);
			std::string ret(temp.ToString());
			FnFree(__int64(temp.c_str()));

			return ret;
		}
	};

	void* (__fastcall* ProcessEventOG)(void* Object, void* Function, void* Params);

	struct UObject
	{
		void** VTable;
		int32_t ObjectFlags;
		int32_t InternalIndex;
		UObject* Class;
		FName Name;
		UObject* Outer;

		uintptr_t GetAddress() {
			return __int64(this);
		}

		bool IsA(UObject* cmp) const
		{
			if (cmp == Class)
				return false;
			return false;
		}

		void* ProcessEvent(UObject* Function, void* Params = nullptr) {
			return ProcessEventOG(this, Function, Params);
		}

		std::string GetName() {
			return GetObjectFullName(this).ToString();
		}

		std::string GetNameOld() {
			return Name.GetName();
		}

		//Use GetName Instead!
		std::string GetFullName() {
			std::string temp;

			for (auto outer = Outer; outer; outer = outer->Outer)
			{
				temp = outer->Name.GetName() + "." + temp;
			}

			temp = reinterpret_cast<UObject*>(Class)->Name.GetName() + " " + temp + this->Name.GetName();

			return temp;
		}
	};

	struct FGuid
	{
		int A;
		int B;
		int C;
		int D;
	};

	struct UObjectItem
	{
		UObject* Object;
		DWORD Flags;
		DWORD ClusterIndex;
		DWORD SerialNumber;
	};

	struct PreUObjectItem
	{
		UObjectItem* FUObject[10];
	};

	class NewUObjectArray {
	public:
		UObjectItem* Objects[9];
	};

	struct GObjects
	{
		NewUObjectArray* ObjectArray;
		BYTE _padding_0[0xC];
		uint32_t NumElements;

		inline void NumChunks(int* start, int* end) const
		{
			int cStart = 0, cEnd = 0;

			if (!cEnd)
			{
				while (1)
				{
					if (ObjectArray->Objects[cStart] == 0)
					{
						cStart++;
					}
					else
					{
						break;
					}
				}

				cEnd = cStart;
				while (1)
				{
					if (ObjectArray->Objects[cEnd] == 0)
					{
						break;
					}
					else
					{
						cEnd++;
					}
				}
			}

			*start = cStart;
			*end = cEnd;
		}

		inline int32_t Num() const
		{
			return NumElements;
		}

		inline UObject* GetByIndex(int32_t index) const
		{
			int cStart = 0, cEnd = 0;
			int chunkIndex = 0, chunkSize = 0xFFFF, chunkPos;
			UObjectItem* Object;

			NumChunks(&cStart, &cEnd);

			chunkIndex = index / chunkSize;
			if (chunkSize * chunkIndex != 0 &&
				chunkSize * chunkIndex == index)
			{
				chunkIndex--;
			}

			chunkPos = cStart + chunkIndex;
			if (chunkPos < cEnd)
			{
				Object = ObjectArray->Objects[chunkPos] + (index - chunkSize * chunkIndex);
				return Object->Object;
			}

			return nullptr;
		}
	};

	class UObjectArray {
	public:
		inline int Num() const
		{
			return NumElements;
		}

		inline UObject* GetByIndex(int32_t index) const
		{
			return (&Objects[index])->Object;
		}

	private:
		UObjectItem* Objects;
		int MaxElements;
		int NumElements;
	};

	struct FVector {
		float X;
		float Y;
		float Z;

		FVector() {
			X = Y = Z = 0;
		}

		FVector(float NX, float NY, float NZ) {
			X = NX;
			Y = NY;
			Z = NZ;
		}
	};

	template <class TEnum>
	class TEnumAsByte
	{
	public:
		TEnumAsByte()
		{
		}

		TEnumAsByte(TEnum _value)
			: value(static_cast<uint8_t>(_value))
		{
		}

		explicit TEnumAsByte(int32_t _value)
			: value(static_cast<uint8_t>(_value))
		{
		}

		explicit TEnumAsByte(uint8_t _value)
			: value(_value)
		{
		}

		operator TEnum() const
		{
			return static_cast<TEnum>(value);
		}

		TEnum GetValue() const
		{
			return static_cast<TEnum>(value);
		}

	private:
		uint8_t value;
	};
}

enum class EFortCustomPartType : uint8_t
{
	Head = 0,
	Body = 1,
	Hat = 2,
	Backpack = 3,
	Charm = 4,
	Face = 5,
	NumTypes = 6,
	EFortCustomPartType_MAX = 7
};

struct Pointer {
	Unreal::UObject* Value;
};

Unreal::UObject* (__fastcall* GFPC)(Unreal::UObject* World);

namespace Game {
	//Vars
	bool InGame = false;
	//Main
	Unreal::GObjects* GObjs;
	Unreal::UObjectArray* ObjObjects;
	Unreal::UObject* GWorld;

	//In Game
	Unreal::UObject* GPC;
	Unreal::UObject* GPawn;
	Unreal::UObject* CM;
}

struct BitField
{
	unsigned char A : 1;
	unsigned char B : 1;
	unsigned char C : 1;
	unsigned char D : 1;
	unsigned char E : 1;
	unsigned char F : 1;
	unsigned char G : 1;
	unsigned char H : 1;
};

enum class ESpawnActorCollisionHandlingMethod : uint8_t
{
	Undefined = 0,
	AlwaysSpawn = 1,
	AdjustIfPossibleButAlwaysSpawn = 2,
	AdjustIfPossibleButDontSpawnIfColliding = 3,
	DontSpawnIfColliding = 4,
	ESpawnActorCollisionHandlingMethod_MAX = 5
};

struct FActorSpawnParameters
{
	FActorSpawnParameters() : Name(), Template(nullptr), Owner(nullptr), Instigator(nullptr), OverrideLevel(nullptr), SpawnCollisionHandlingOverride(), bRemoteOwned(0), bNoFail(0),
		bDeferConstruction(0),
		bAllowDuringConstructionScript(0),
		NameMode(),
		ObjectFlags()
	{
	}
	;


	Unreal::FName Name;

	Unreal::UObject* Template;

	Unreal::UObject* Owner;

	Unreal::UObject* Instigator;

	Unreal::UObject* OverrideLevel;

	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;

private:

	uint8_t bRemoteOwned : 1;

public:

	bool IsRemoteOwned() const { return bRemoteOwned; }

	uint8_t bNoFail : 1;


	uint8_t bDeferConstruction : 1;

	uint8_t bAllowDuringConstructionScript : 1;


	enum class ESpawnActorNameMode : uint8_t
	{
		Required_Fatal,

		Required_ErrorAndReturnNull,

		Required_ReturnNull,

		Requested
	};


	ESpawnActorNameMode NameMode;

	uint8_t ObjectFlags;
};
Unreal::UObject* (__fastcall* StaticLoadObjectInternal)(Unreal::UObject*, Unreal::UObject*, const TCHAR*, const TCHAR*, uint32_t, Unreal::UObject*, bool);
Unreal::UObject* (__fastcall* StaticConstructObjectInternal)(void*, void*, void*, int, unsigned int, void*, bool, void*, bool);
Unreal::UObject* (__fastcall* SpawnActor)(Unreal::UObject* World, Unreal::UObject* Class, Unreal::FVector* Loc, Unreal::FVector* Rot, const FActorSpawnParameters& SpawnParameters);

Unreal::UObject* FindObject(std::string TargetName, bool Equals = false) {
	if (GVersion < 5.0f) {
		for (int i = 0; i < Game::ObjObjects->Num(); i++) {
			Unreal::UObject* Object = Game::ObjObjects->GetByIndex(i);
			std::string ObjName = Object->GetName();
			if (ObjName == TargetName && Equals == true) {
				return Object;
			}
			if (ObjName.find(TargetName) != std::string::npos && Equals == false) {
				return Object;
			}
		}
	}
	else {
		for (int i = 0; i < Game::GObjs->Num(); i++) {
			Unreal::UObject* Object = Game::GObjs->GetByIndex(i);
			std::string ObjName = Object->GetName();
			if (ObjName == TargetName && Equals == true) {
				return Object;
			}
			if (ObjName.find(TargetName) != std::string::npos && Equals == false) {
				return Object;
			}
		}
	}
	return nullptr;
}

void DumpObjects() {
	std::ofstream log("Objects.txt");
	if (GVersion < 5.0f) {
		for (int i = 0; i < Game::ObjObjects->Num(); i++) {
			Unreal::UObject* Object = Game::ObjObjects->GetByIndex(i);
			std::string ObjName = Object->GetName();
			std::string item = "\nName: " + ObjName;
			log << item;
		}
	}
	else {
		for (int i = 0; i < Game::GObjs->Num(); i++) {
			Unreal::UObject* Object = Game::GObjs->GetByIndex(i);
			std::string ObjName = Object->GetName();
			std::string item = "\nName: " + ObjName;
			log << item;
		}
	}
}

using namespace Unreal;

enum class ENetRole : uint8_t
{
	ROLE_None = 0,
	ROLE_SimulatedProxy = 1,
	ROLE_AutonomousProxy = 2,
	ROLE_Authority = 3,
	ROLE_MAX = 4
};

struct FURL
{
	struct FString                                     Protocol;                                                 // 0x0000(0x0010) (ZeroConstructor)
	struct FString                                     Host;                                                     // 0x0010(0x0010) (ZeroConstructor)
	int                                                Port;                                                     // 0x0020(0x0004) (ZeroConstructor, IsPlainOldData)
	int                                                Valid;                                                    // 0x0024(0x0004) (ZeroConstructor, IsPlainOldData)
	struct FString                                     Map;                                                      // 0x0028(0x0010) (ZeroConstructor)
	struct FString                                     RedirectUrl;                                              // 0x0038(0x0010) (ZeroConstructor)
	TArray<struct FString>                             Op;                                                       // 0x0048(0x0010) (ZeroConstructor)
	struct FString                                     Portal;                                                   // 0x0058(0x0010) (ZeroConstructor)
};

enum ENetMode
{
	NM_Standalone,
	NM_DedicatedServer,
	NM_ListenServer,
	NM_Client,
	NM_MAX,
};

enum class EFortTeam : uint8_t
{
	HumanCampaign = 0,
	Monster = 1,
	HumanPvP_Team1 = 2,
	HumanPvP_Team2 = 3,
	HumanPvP_Team3 = 4,
	HumanPvP_Team4 = 5,
	HumanPvP_Team5 = 6,
	HumanPvP_Team6 = 7,
	HumanPvP_Team7 = 8,
	HumanPvP_Team8 = 9,
	HumanPvP_Team9 = 10,
	HumanPvP_Team10 = 11,
	HumanPvP_Team11 = 12,
	HumanPvP_Team12 = 13,
	HumanPvP_Team13 = 14,
	HumanPvP_Team14 = 15,
	HumanPvP_Team15 = 16,
	HumanPvP_Team16 = 17,
	HumanPvP_Team17 = 18,
	HumanPvP_Team18 = 19,
	HumanPvP_Team19 = 20,
	HumanPvP_Team20 = 21,
	HumanPvP_Team21 = 22,
	HumanPvP_Team22 = 23,
	HumanPvP_Team23 = 24,
	HumanPvP_Team24 = 25,
	HumanPvP_Team25 = 26,
	HumanPvP_Team26 = 27,
	HumanPvP_Team27 = 28,
	HumanPvP_Team28 = 29,
	HumanPvP_Team29 = 30,
	HumanPvP_Team30 = 31,
	HumanPvP_Team31 = 32,
	HumanPvP_Team32 = 33,
	HumanPvP_Team33 = 34,
	HumanPvP_Team34 = 35,
	HumanPvP_Team35 = 36,
	HumanPvP_Team36 = 37,
	HumanPvP_Team37 = 38,
	HumanPvP_Team38 = 39,
	HumanPvP_Team39 = 40,
	HumanPvP_Team40 = 41,
	HumanPvP_Team41 = 42,
	HumanPvP_Team42 = 43,
	HumanPvP_Team43 = 44,
	HumanPvP_Team44 = 45,
	HumanPvP_Team45 = 46,
	HumanPvP_Team46 = 47,
	HumanPvP_Team47 = 48,
	HumanPvP_Team48 = 49,
	HumanPvP_Team49 = 50,
	HumanPvP_Team50 = 51,
	HumanPvP_Team51 = 52,
	HumanPvP_Team52 = 53,
	HumanPvP_Team53 = 54,
	HumanPvP_Team54 = 55,
	HumanPvP_Team55 = 56,
	HumanPvP_Team56 = 57,
	HumanPvP_Team57 = 58,
	HumanPvP_Team58 = 59,
	HumanPvP_Team59 = 60,
	HumanPvP_Team60 = 61,
	HumanPvP_Team61 = 62,
	HumanPvP_Team62 = 63,
	HumanPvP_Team63 = 64,
	HumanPvP_Team64 = 65,
	HumanPvP_Team65 = 66,
	HumanPvP_Team66 = 67,
	HumanPvP_Team67 = 68,
	HumanPvP_Team68 = 69,
	HumanPvP_Team69 = 70,
	HumanPvP_Team70 = 71,
	HumanPvP_Team71 = 72,
	HumanPvP_Team72 = 73,
	HumanPvP_Team73 = 74,
	HumanPvP_Team74 = 75,
	HumanPvP_Team75 = 76,
	HumanPvP_Team76 = 77,
	HumanPvP_Team77 = 78,
	HumanPvP_Team78 = 79,
	HumanPvP_Team79 = 80,
	HumanPvP_Team80 = 81,
	HumanPvP_Team81 = 82,
	HumanPvP_Team82 = 83,
	HumanPvP_Team83 = 84,
	HumanPvP_Team84 = 85,
	HumanPvP_Team85 = 86,
	HumanPvP_Team86 = 87,
	HumanPvP_Team87 = 88,
	HumanPvP_Team88 = 89,
	HumanPvP_Team89 = 90,
	HumanPvP_Team90 = 91,
	HumanPvP_Team91 = 92,
	HumanPvP_Team92 = 93,
	HumanPvP_Team93 = 94,
	HumanPvP_Team94 = 95,
	HumanPvP_Team95 = 96,
	HumanPvP_Team96 = 97,
	HumanPvP_Team97 = 98,
	HumanPvP_Team98 = 99,
	HumanPvP_Team99 = 100,
	HumanPvP_Team100 = 101,
	HumanPvP_Team101 = 102,
	Spectator = 103,
	MAX = 104
};

struct alignas(16) FQuat
{
	float                                              X;                                                        // 0x0000(0x0004) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	float                                              Y;                                                        // 0x0004(0x0004) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	float                                              Z;                                                        // 0x0008(0x0004) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	float                                              W;                                                        // 0x000C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
};

struct alignas(16) FTransform
{
	struct FQuat                                       Rotation;                                                 // 0x0000(0x0010) (Edit, BlueprintVisible, SaveGame, IsPlainOldData)
	struct FVector                                     Translation;                                              // 0x0010(0x000C) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	unsigned char                                      UnknownData00[0x4];                                       // 0x001C(0x0004) MISSED OFFSET
	struct FVector                                     Scale3D;                                                  // 0x0020(0x000C) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	unsigned char                                      UnknownData01[0x4];                                       // 0x002C(0x0004) MISSED OFFSET
};

struct UGameplayStatics_BeginDeferredActorSpawnFromClass_Params
{
	class UObject* WorldContextObject;                                       // (ConstParm, Parm, ZeroConstructor, IsPlainOldData)
	class UObject* ActorClass;                                               // (Parm, ZeroConstructor, IsPlainOldData)
	struct FTransform                                  SpawnTransform;                                           // (ConstParm, Parm, OutParm, ReferenceParm, IsPlainOldData)
	ESpawnActorCollisionHandlingMethod                 CollisionHandlingOverride;                                // (Parm, ZeroConstructor, IsPlainOldData)
	class UObject* Owner;                                                    // (Parm, ZeroConstructor, IsPlainOldData)
	class UObject* ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData)
};

struct UGameplayStatics_FinishSpawningActor_Params
{
	class UObject* Actor;                                                    // (Parm, ZeroConstructor, IsPlainOldData)
	struct FTransform                                  SpawnTransform;                                           // (ConstParm, Parm, OutParm, ReferenceParm, IsPlainOldData)
	class UObject* ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData)
};