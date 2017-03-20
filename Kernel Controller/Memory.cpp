#include "Memory.h"
#include <iostream>
#include <cassert>

float reinterpret_itof(uint32_t a) {
	float b;
	char* aPointer = (char*)&a, *bPointer = (char*)&b;
	memcpy(bPointer, aPointer, sizeof(a));
	return b;
}

Memory::Memory(LPCSTR RegistryPath)
{
	hDriver = CreateFileA(RegistryPath, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	DwClient = GetClientModule();
	DwEngine = GetEngineModule();
}

DWORD Memory::GetClientState()
{
	return ReadVirtualMemory<DWORD>( DwEngine + DwClientState - offset, sizeof(DWORD));
}

DWORD Memory::GetLocalPlayer()
{
	return ReadVirtualMemory<DWORD>( DwClient + DwLocalPlayer - offset, sizeof(DWORD));
}

DWORD Memory::GetEntity(int PlayerNumber)
{
	return ReadVirtualMemory<DWORD>( DwClient + DwEntityList - offset + (0x10 * PlayerNumber), sizeof(DWORD));
}

void Memory::GetBonePosition(float* BonePosition, int TargetBone, int PlayerNumber)
{
	DWORD BoneMatrix, Entity;
	if (PlayerNumber == MaxPlayer)
		Entity = GetLocalPlayer();
	else
		Entity = GetEntity(PlayerNumber);
	BoneMatrix = ReadVirtualMemory<DWORD>( Entity + DwBoneMatrix - offset, sizeof(DWORD));
	BonePosition[0] = reinterpret_itof(ReadVirtualMemory<DWORD>( BoneMatrix + 0x30 * TargetBone + 0x0C, sizeof(float)));
	BonePosition[1] = reinterpret_itof(ReadVirtualMemory<DWORD>( BoneMatrix + 0x30 * TargetBone + 0x1C, sizeof(float)));
	BonePosition[2] = reinterpret_itof(ReadVirtualMemory<DWORD>( BoneMatrix + 0x30 * TargetBone + 0x2C, sizeof(float)));
}

void Memory::GetPosition(float* Position, int PlayerNumber)
{
	DWORD Entity;
	if (PlayerNumber == MaxPlayer)
		Entity = GetLocalPlayer();
	else
		Entity = GetEntity(PlayerNumber);
	for (int i = 0; i<3; i++)
		Position[i] = reinterpret_itof(ReadVirtualMemory<DWORD>( Entity + DwVecOrigin - offset + sizeof(float)*i, sizeof(float)));
}

/*void Memory::GetVelocity(float* Velocity, int PlayerNumber)
{
DWORDLONG Entity;
if (PlayerNumber == GetMaxPlayer())
Entity = GetLocalPlayer();
else
Entity = GetEntity(PlayerNumber);
ReadProcessMemory(_process, (LPVOID)(Entity + DwVecVelocity), Velocity, sizeof(float) * 3, NULL);
}*/

void Memory::GetAngles(float* Angles)
{
	for (int i = 0; i<3; i++)
		Angles[i] = reinterpret_itof(ReadVirtualMemory<DWORD>( GetClientState() + DwViewAngle - offset + sizeof(float)*i, sizeof(float)));
}

void Memory::GetPunch(float* Punch)
{
	for (int i = 0; i<2; i++)
		Punch[i] = reinterpret_itof(ReadVirtualMemory<DWORD>( GetLocalPlayer() + DwVecPunch - offset + sizeof(float)*i, sizeof(float)));
}

void Memory::GetViewOrigin(float* ViewOrigin)
{
	for (int i = 0; i<3; i++)
		ViewOrigin[i] = reinterpret_itof(ReadVirtualMemory<DWORD>( GetLocalPlayer() + DwVecViewOffset - offset + sizeof(float)*i, sizeof(float)));
}

int Memory::GetFlags()
{
	return ReadVirtualMemory<int>( GetLocalPlayer() + DwFlags - offset, sizeof(int));
}

int Memory::GetTeam(int PlayerNumber)
{
	DWORD Entity;
	if (PlayerNumber == MaxPlayer)
		Entity = GetLocalPlayer();
	else
		Entity = GetEntity(PlayerNumber);
	return (int)ReadVirtualMemory<DWORD>( Entity + DwTeamNumber - offset, sizeof(DWORD));
}

int Memory::GetHealth(int PlayerNumber)
{
	DWORD Entity;
	if (PlayerNumber == MaxPlayer)
		Entity = GetLocalPlayer();
	else
		Entity = GetEntity(PlayerNumber);
	return (int)ReadVirtualMemory<DWORD>( Entity + DwHealth - offset, sizeof(DWORD));
}

int Memory::IsDead(int PlayerNumber)
{
	DWORD Entity;
	if (PlayerNumber == MaxPlayer)
		Entity = GetLocalPlayer();
	else
		Entity = GetEntity(PlayerNumber);
	return (int)ReadVirtualMemory<DWORD>( Entity + DwLifeState - offset, sizeof(DWORD));
}

bool Memory::IsScoped()
{
	return (bool)ReadVirtualMemory<DWORD>( GetLocalPlayer() + DwScoped - offset, sizeof(DWORD));
}

bool Memory::IsDormant(int PlayerNumber)
{
	DWORD Entity;
	if (PlayerNumber == MaxPlayer)
		Entity = GetLocalPlayer();
	else
		Entity = GetEntity(PlayerNumber);
	return (bool)ReadVirtualMemory<DWORD>( Entity + DwDormant - offset, sizeof(DWORD));
}

int Memory::GetShotsFired()
{
	return ReadVirtualMemory<int>( GetLocalPlayer() + DwShotsFired - offset, sizeof(int));
}

DWORD Memory::GetWeapon()
{
	DWORD PlayerWeaponIndex;
	PlayerWeaponIndex = ReadVirtualMemory<DWORD>( GetLocalPlayer() + DwActiveWeapon - offset, sizeof(DWORD));
	PlayerWeaponIndex &= 0xFFF;
	return ReadVirtualMemory<DWORD>( DwClient + DwEntityList - offset + (PlayerWeaponIndex * 0x10) - 0x10, sizeof(DWORD));
}

int Memory::GetWeaponId() {
	return ReadVirtualMemory<int>( GetWeapon() + DwWeaponId - offset, sizeof(int));
}

int Memory::GetWeaponClip() {
	return ReadVirtualMemory<int>( GetWeapon() + DwClip - offset, sizeof(int));
}

bool Memory::GetWeaponInReload()
{
	return (bool)ReadVirtualMemory<DWORD>( GetWeapon() + DwInReload - offset, sizeof(DWORD));
}

void Memory::GetMapName(char* MapName) {

	for(int i=0; i<128; i++)
		MapName[i] = ReadVirtualMemory<char>( GetClientState() + DwMapname - offset + i * sizeof(char), sizeof(char));
}

int Memory::GetStatus() {
	return ReadVirtualMemory<int>( GetClientState() + DwInGame - offset, sizeof(int));
}