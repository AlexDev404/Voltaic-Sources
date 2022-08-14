#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

#include "SDK/SDK.hpp"
#include "minhook/minhook.h"
#include "memory.h"
#include "math.h"
#include "core.h"

void Main()
{
    AllocConsole();
    FILE* pFile;
    freopen_s(&pFile, "CONOUT$", "w", stdout);

    std::cout << "Initializing" << "\n";

    UObject::GObjects = decltype(UObject::GObjects)(Memory::FindPattern("48 8B 0D ? ? ? ? 48 98 4C 8B 04 D1 48 8D 0C 40 49 8D 04 C8 EB ? 48 8B C6 8B 40 08 C1 E8 ? A8 ? 0F 85 ? ? ? ? 48 8B 55 F8", 7));
    FMemory_Free = decltype(FMemory_Free)(Memory::FindPattern("48 85 C9 74 ? 53 48 83 EC ? 48 8B D9 48 8B 0D"));
    FMemory_Malloc = decltype(FMemory_Malloc)(Memory::FindPattern("48 89 5C 24 08 57 48 83 EC ? 48 8B F9 8B DA 48 8B 0D ? ? ? ? 48 85 C9"));
    FMemory_Realloc = decltype(FMemory_Realloc)(Memory::FindPattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 48 8B F1 41 8B D8 48 8B 0D"));
    FNameToString = decltype(FNameToString)(Memory::FindPattern("48 89 5C 24 08 57 48 83 EC ? 83 79 04 ? 48 8B DA"));

    MH_Initialize();

    std::cout << "Switching Level" << "\n";
    Core::GetWorld()->OwningGameInstance->LocalPlayers[0]->PlayerController->SwitchLevel(L"Athena_Terrain?game=/Game/Athena/Athena_GameMode.Athena_GameMode_C");

    Core::Initialize();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        Main();

    return TRUE;
}