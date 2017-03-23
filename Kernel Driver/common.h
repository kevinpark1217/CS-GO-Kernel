#pragma once
#include <NTddk.h> 
//#include <ntifs.h>

typedef struct _DIRECTORY_BASIC_INFORMATION
{
	UNICODE_STRING ObjectName;
	UNICODE_STRING ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION, *PDIRECTORY_BASIC_INFORMATION;

struct DEVOBJ_EXTENSION_FIX
{
	USHORT type;
	USHORT size;
	PDEVICE_OBJECT devObj;
	ULONGLONG PowerFlags;
	void *Dope;
	ULONGLONG ExtensionFlags;
	void *DeviceNode;
	PDEVICE_OBJECT AttachedTo;
};

typedef struct _ARKNTAPI {
	ULONG dwNtOpenDirectoryObject;
	ULONG dwNtQueryDirectoryObject;
} ARKNTAPI, *PARKNTAPI;

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES
	ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
	IN HANDLE  DirectoryHandle,
	OUT PVOID  Buffer,
	IN ULONG   BufferLength,
	IN BOOLEAN ReturnSingleEntry,
	IN BOOLEAN RestartScan,
	IN OUT PULONG  Context,
	OUT PULONG ReturnLength OPTIONAL
);

typedef NTSTATUS(*IRPMJREAD) (IN PDEVICE_OBJECT, IN PIRP);

void *FindDevNodeRecurse(PDEVICE_OBJECT a1, ULONGLONG *a2);