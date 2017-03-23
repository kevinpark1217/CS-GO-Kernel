#ifndef MEMORY_H
#define MEMORY_H

//#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <Windows.h>


// Request to read virtual user memory (memory of a program) from kernel space
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5851 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to write virtual user memory (memory of a program) from kernel space
//#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5852 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to retrieve the base address of client.dll in csgo.exe from kernel space
#define IO_GET_CLIENT_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5854 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to retrieve the base address of engine.dll in csgo.exe from kernel space
#define IO_GET_ENGINE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5855 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Send mouse data to kernel space for stream injection
#define IO_MOUSE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5856 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct _MOUSE_REQUEST
{
	BOOLEAN click;
	BOOLEAN status;
	LONG dx;
	LONG dy;

} MOUSE_REQUEST, *PMOUSE_REQUEST;


typedef struct _KERNEL_READ_REQUEST
{
	DWORDLONG Address;
	ULONGLONG Response;
	SIZE_T Size;

} KERNEL_READ_REQUEST, *PKERNEL_READ_REQUEST;
/*
typedef struct _KERNEL_WRITE_REQUEST
{
	DWORDLONG Address;
	ULONGLONG Value;
	SIZE_T Size;

} KERNEL_WRITE_REQUEST, *PKERNEL_WRITE_REQUEST;
*/
class Memory
{
private:

	//Dll Offsets
	DWORD DwClient = 0x0;
	DWORD DwEngine = 0x0;

	const DWORD offset = 0x8903;

	//Offsets
	const DWORD DwLocalPlayer = 0xab6007;
	const DWORD DwEntityList = 0x4ad9187;
	const DWORD DwClientState = 0x5d3e27;
	const DWORD DwViewAngle = 0xd60f;
	const DWORD DwVecViewOffset = 0x8a07;
	const DWORD DwVecOrigin = 0x8a37;
	const DWORD DwVecPunch = 0xb91f;
	const DWORD DwScoped = 0xc19f;
	const DWORD DwTeamNumber = 0x89f3;
	const DWORD DwShotsFired = 0x12bc3;
	const DWORD DwBoneMatrix = 0xaf9b;
	const DWORD DwHealth = 0x89ff;
	const DWORD DwLifeState = 0x8b5e;
	const DWORD DwDormant = 0x89ec;
	const DWORD DwActiveWeapon = 0xb7eb;
	const DWORD DwFlags = 0x8a03;
	const DWORD DwMapname = 0x8b87;
	const DWORD DwWeaponId = 0xbbef + 0x4;
	const DWORD DwClip = 0xbb07;
	const DWORD DwInReload = 0xbb48;
	const DWORD DwInGame = 0x8a03;

public:

	HANDLE hDriver;

	template <typename type>
	type ReadVirtualMemory(DWORD ReadAddress, SIZE_T Size)
	{
		if (hDriver == INVALID_HANDLE_VALUE)
			return (type)false;

		DWORD Return, Bytes;
		KERNEL_READ_REQUEST ReadRequest;

		ReadRequest.Address = ReadAddress;
		ReadRequest.Size = Size;

		// send code to our driver with the arguments
		if (DeviceIoControl(hDriver, IO_READ_REQUEST, &ReadRequest, sizeof(ReadRequest), &ReadRequest, sizeof(ReadRequest), 0, 0))
			return (type)ReadRequest.Response;
		else
			return (type)false;
	}
	/*
	bool WriteVirtualMemory(DWORD WriteAddress, ULONG WriteValue, SIZE_T WriteSize)
	{
		if (hDriver == INVALID_HANDLE_VALUE)
			return false;
		DWORD Bytes;

		KERNEL_WRITE_REQUEST  WriteRequest;
		WriteRequest.Address = WriteAddress;
		WriteRequest.Value = WriteValue;
		WriteRequest.Size = WriteSize;

		if (DeviceIoControl(hDriver, IO_WRITE_REQUEST, &WriteRequest, sizeof(WriteRequest), 0, 0, &Bytes, NULL))
			return true;
		else
			return false;
	}
	*/
	DWORD GetClientModule()
	{
		if (hDriver == INVALID_HANDLE_VALUE)
			return false;

		DWORDLONG Address;
		DWORD Bytes;

		if (DeviceIoControl(hDriver, IO_GET_CLIENT_REQUEST, &Address, sizeof(Address), &Address, sizeof(Address), &Bytes, NULL))
			return (DWORD)Address;
		else
			return false;
	}

	DWORD GetEngineModule()
	{
		if (hDriver == INVALID_HANDLE_VALUE)
			return false;

		DWORDLONG Address;
		DWORD Bytes;

		if (DeviceIoControl(hDriver, IO_GET_ENGINE_REQUEST, &Address, sizeof(Address), &Address, sizeof(Address), &Bytes, NULL))
			return (DWORD)Address;
		else
			return false;
	}

	void MouseMove(float x, float y)
	{
		INPUT Input = { 0 };
		Input.type = INPUT_MOUSE;
		Input.mi.dx = (LONG)(x * 10);
		Input.mi.dy = (LONG)(y * 10);
		Input.mi.dwFlags = MOUSEEVENTF_MOVE;
		SendInput(1, &Input, sizeof(INPUT));
		/*DWORD Bytes;
		MOUSE_INPUT_DATA Input = { 0 };
		Input.Flags = MOUSE_MOVE_ABSOLUTE;
		Input.LastX = (LONG)(x * 10);
		Input.LastY = (LONG)(y * 10);
		DeviceIoControl(hDriver, IO_MOUSE_REQUEST, &Input, sizeof(Input), &Input, sizeof(Input), &Bytes, NULL);*/
	}


	void Shoot()
	{
		DWORD Bytes;
		MOUSE_REQUEST Input;
		Input.click = true;
		Input.status = 1;
		DeviceIoControl(hDriver, IO_MOUSE_REQUEST, &Input, sizeof(Input), 0, 0, &Bytes, NULL);
		Sleep(rand() % 10 + 175);
		Input.click = true;
		Input.status = 0;
		DeviceIoControl(hDriver, IO_MOUSE_REQUEST, &Input, sizeof(Input), 0, 0, &Bytes, NULL);
		/*SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, 0x02010001);
		PostMessage(hwnd, WM_LBUTTONDOWN, 0x1, 0x021C03C0);
		Sleep(rand() % 10 + 175);
		SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, 0x02020002);
		PostMessage(hwnd, WM_LBUTTONUP, 0x0, 0x021C03C0);*/
	}

	const int MaxPlayer = 64;

	Memory(LPCSTR RegistryPath);
	DWORD GetClientState();
	DWORD GetWeapon();
	DWORD GetLocalPlayer();
	DWORD GetEntity(int PlayerNumber);
	void GetBonePosition(float* BonePosition, int TargetBone, int PlayerNumber);
	void GetPosition(float* Position, int PlayerNumber);
	void GetAngles(float* Angles);
	void GetPunch(float* Punch);
	void GetViewOrigin(float * ViewOrigin);
	int GetFlags();
	int GetTeam(int PlayerNumber);
	int GetHealth(int PlayerNumber);
	int IsDead(int PlayerNumber);
	bool IsDormant(int PlayerNumber);
	bool IsScoped();
	int GetShotsFired();
	int GetWeaponId();
	int GetWeaponClip();
	bool GetWeaponInReload();
	void GetMapName(char* MapName);
	int GetStatus();
};

#endif 