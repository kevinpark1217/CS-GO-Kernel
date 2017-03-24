#include <ntdef.h>
#include <ntifs.h>
#include <ntddk.h>
#include <Kbdmou.h>
//#include "Memory.h"
//#include "keyboardhook.h"

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

#define MOUCLASS_CONNECT_REQUEST 0x0F0203
#define MOU_STRING_INC 0x14

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

typedef NTSTATUS(__fastcall *MouclassRead)(PDEVICE_OBJECT device, PIRP irp);
typedef void(__fastcall *MouseServiceDpc)(PDEVICE_OBJECT mou, PMOUSE_INPUT_DATA a1, PMOUSE_INPUT_DATA a2, PULONG a3);
typedef NTSTATUS(__fastcall *MouseAddDevice)(PDRIVER_OBJECT a1, PDEVICE_OBJECT a2);
typedef NTSTATUS(__fastcall *mouinput)(void *a1, void *a2, void *a3, void *a4, void *a5);

PDEVICE_OBJECT pDeviceObject; // our driver object
PDEVICE_OBJECT pMouseObject;
PDEVICE_OBJECT pMouseTarget;
UNICODE_STRING dev, dos; // Driver registry paths

char MOU_DATA[5];

HANDLE ProcessHandle;
ULONGLONG ClientAddress, EngineAddress;
ULONG mouId = 0;

MouclassRead MouClassReadRoutine;
MouseServiceDpc MouseDpcRoutine;
mouinput MouseInputRoutine = NULL;
MOUSE_INPUT_DATA mdata;
PMOUSE_INPUT_DATA mouIrp = NULL;

// datatype for read request
typedef struct _KERNEL_READ_REQUEST
{
	ULONGLONG Address;
	ULONGLONG Response;
	SIZE_T Size;

} KERNEL_READ_REQUEST, *PKERNEL_READ_REQUEST;

typedef struct _MOUSE_REQUEST
{
	BOOLEAN click;
	BOOLEAN status;
	LONG dx;
	LONG dy;

} MOUSE_REQUEST, *PMOUSE_REQUEST;

/*typedef struct _KERNEL_WRITE_REQUEST
{
	ULONGLONG Address;
	ULONGLONG Value;
	SIZE_T Size;

} KERNEL_WRITE_REQUEST, *PKERNEL_WRITE_REQUEST;*/

NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriverObject);
NTSTATUS CreateCall(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS CloseCall(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS NTAPI MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess, PVOID TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
NTKERNELAPI PDEVICE_OBJECT IoGetBaseFileSystemDeviceObject(_In_ PFILE_OBJECT FileObject);

NTSTATUS KeReadVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
	PSIZE_T Bytes;
	if (NT_SUCCESS(MmCopyVirtualMemory(Process, SourceAddress, PsGetCurrentProcess(), TargetAddress, Size, KernelMode, &Bytes)))
		return STATUS_SUCCESS;
	else
		return STATUS_ACCESS_DENIED;
}

NTSTATUS KeWriteVirtualMemory(PEPROCESS Process, PVOID SourceAddress, PVOID TargetAddress, SIZE_T Size)
{
	PSIZE_T Bytes;
	if (NT_SUCCESS(MmCopyVirtualMemory(PsGetCurrentProcess(), SourceAddress, Process, TargetAddress, Size, KernelMode, &Bytes)))
		return STATUS_SUCCESS;
	else
		return STATUS_ACCESS_DENIED;
}

void Xor(wchar_t* text, int length) {
	char key = 'P'; //Any char will work

	for (int i = 0; i < length; i++)
	{
		text[i] ^= key;
	}
}

NTSTATUS MouseApc(void *a1, void *a2, void *a3, void *a4, void *a5)
{
	if (mouIrp->ButtonFlags&MOUSE_LEFT_BUTTON_DOWN)
	{
		MOU_DATA[0] = 1;
	}
	else if (mouIrp->ButtonFlags&MOUSE_LEFT_BUTTON_UP)
	{
		MOU_DATA[0] = 0;
	}
	else if (mouIrp->ButtonFlags&MOUSE_RIGHT_BUTTON_DOWN)
	{
		MOU_DATA[1] = 1;
	}
	else if (mouIrp->ButtonFlags&MOUSE_RIGHT_BUTTON_UP)
	{
		MOU_DATA[1] = 0;
	}
	else if (mouIrp->ButtonFlags&MOUSE_MIDDLE_BUTTON_DOWN)
	{
		MOU_DATA[2] = 1;
	}
	else if (mouIrp->ButtonFlags&MOUSE_MIDDLE_BUTTON_UP)
	{
		MOU_DATA[2] = 0;
	}
	else if (mouIrp->ButtonFlags&MOUSE_BUTTON_4_DOWN)
	{
		MOU_DATA[3] = 1;
	}
	else if (mouIrp->ButtonFlags&MOUSE_BUTTON_4_UP)
	{
		MOU_DATA[3] = 0;
	}
	else if (mouIrp->ButtonFlags&MOUSE_BUTTON_5_DOWN)
	{
		MOU_DATA[4] = 1;
	}
	else if (mouIrp->ButtonFlags&MOUSE_BUTTON_5_UP)
	{
		MOU_DATA[4] = 0;
	}

	return MouseInputRoutine(a1, a2, a3, a4, a5);
}

void *FindDevNodeRecurse(PDEVICE_OBJECT a1, ULONGLONG *a2)
{
	struct DEVOBJ_EXTENSION_FIX *attachment;
	attachment = a1->DeviceObjectExtension;
	if ((!attachment->AttachedTo) && (!attachment->DeviceNode)) return;

	if ((!attachment->DeviceNode) && (attachment->AttachedTo))
	{
		FindDevNodeRecurse(attachment->AttachedTo, a2);

		return;
	}

	*a2 = (ULONGLONG)attachment->DeviceNode;

	return;
}

void SynthesizeMouse(PMOUSE_INPUT_DATA a1)
{
	KIRQL irql;
	char *endptr;
	ULONG fill = 1;

	endptr = (char*)a1;

	endptr += sizeof(MOUSE_INPUT_DATA);

	a1->UnitId = mouId;

	KeRaiseIrql(DISPATCH_LEVEL, &irql);

	MouseDpcRoutine(pMouseTarget, a1, (PMOUSE_INPUT_DATA)endptr, &fill);

	KeLowerIrql(irql);

}

NTSTATUS My_IoGetDeviceObjectPointer(IN PUNICODE_STRING ObjectName, IN ACCESS_MASK DesiredAccess, OUT PFILE_OBJECT *FileObject, OUT PDEVICE_OBJECT *DeviceObject)
{
	PFILE_OBJECT fileObject;
	OBJECT_ATTRIBUTES objectAttributes;
	HANDLE fileHandle;
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS status;

	InitializeObjectAttributes(&objectAttributes, ObjectName, OBJ_KERNEL_HANDLE, (HANDLE)NULL, (PSECURITY_DESCRIPTOR)NULL);

	status = ZwOpenFile(&fileHandle, DesiredAccess, &objectAttributes, &ioStatus, FILE_SHARE_READ, FILE_NON_DIRECTORY_FILE);

	if (NT_SUCCESS(status))
	{
		status = ObReferenceObjectByHandle(fileHandle, 0, *IoFileObjectType, KernelMode, (PVOID *)&fileObject, NULL);
		if (NT_SUCCESS(status))
		{
			*FileObject = fileObject;
			*DeviceObject = IoGetBaseFileSystemDeviceObject(fileObject);
		}
		ZwClose(fileHandle);
	}
	return status;
}

// set a callback for every PE image loaded to user memory
// then find the client.dll & csgo.exe using the callback
PLOAD_IMAGE_NOTIFY_ROUTINE ImageLoadCallback(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	// Compare our string to input
	wchar_t client[21] = { 12,51,35,55,63,12,50,57,62,12,51,60,57,53,62,36,126,52,60,60,0 };
	wchar_t engine[16] = { 12,50,57,62,12,53,62,55,57,62,53,126,52,60,60,0 };
	Xor(client, 20);
	Xor(engine, 15);
	if (wcsstr(FullImageName->Buffer, client)) {
		// if it matches
		//DbgPrintEx(0, 0, "Loaded Name: %ls \n", FullImageName->Buffer);
		//DbgPrintEx(0, 0, "Loaded To Process: %d \n", ProcessId);

		ClientAddress = ImageInfo->ImageBase;
		ProcessHandle = ProcessId;
	}
	else if (wcsstr(FullImageName->Buffer, engine)) {
		// if it matches
		//DbgPrintEx(0, 0, "Loaded Name: %ls \n", FullImageName->Buffer);
		//DbgPrintEx(0, 0, "Loaded To Process: %d \n", ProcessId);
		
		EngineAddress = ImageInfo->ImageBase;
		//csgoId = ProcessId;
	}
	Xor(client, 20);
	Xor(engine, 15);
}

NTSTATUS ReadMouse(PDEVICE_OBJECT device, PIRP irp)
{
	ULONGLONG *routine;

	routine = (ULONGLONG*)irp;

	routine += 0xb;


	if (!MouseInputRoutine)
	{
		MouseInputRoutine = (mouinput)*routine;
	}

	*routine = (ULONGLONG)MouseApc;

	mouIrp = (struct KEYBOARD_INPUT_DATA*)irp->UserBuffer;

	return MouClassReadRoutine(device, irp);
}

// IOCTL Call Handler function
NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status;
	ULONGLONG BytesIO = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	// Code received from user space
	ULONG ControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
	
	if (ControlCode == IO_READ_REQUEST)
	{
		// Get the input buffer & format it to our struct
		PKERNEL_READ_REQUEST ReadInput = (PKERNEL_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		PEPROCESS Process;
		// Get our process
		if (NT_SUCCESS(PsLookupProcessByProcessId(ProcessHandle, &Process)))
			KeReadVirtualMemory(Process, ReadInput->Address, &ReadInput->Response, ReadInput->Size);

		//DbgPrintEx(0, 0, "Read Params:  %lu, %#010x \n", ReadInput->ProcessId, ReadInput->Address);
		//DbgPrintEx(0, 0, "Value: %lu \n", ReadOutput->Response);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(KERNEL_READ_REQUEST);
	}
	/*else if (ControlCode == IO_WRITE_REQUEST)
	{
		// Get the input buffer & format it to our struct
		PKERNEL_WRITE_REQUEST WriteInput = (PKERNEL_WRITE_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		PEPROCESS Process;
		// Get our process
		if (NT_SUCCESS(PsLookupProcessByProcessId(ProcessHandle, &Process)))
			KeWriteVirtualMemory(Process, &WriteInput->Value, WriteInput->Address, WriteInput->Size);

		//DbgPrintEx(0, 0, "Write Params:  %lu, %#010x \n", WriteInput->Value, WriteInput->Address);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(PKERNEL_WRITE_REQUEST);
	}*/
	else if (ControlCode == IO_GET_CLIENT_REQUEST)
	{
		PULONGLONG OutPut = (PULONGLONG)Irp->AssociatedIrp.SystemBuffer;
		*OutPut = ClientAddress;

		//DbgPrintEx(0, 0, "Client Module get %#010x", ClientAddress);
		Status = STATUS_SUCCESS;
		BytesIO = sizeof(ClientAddress);
	}
	else if (ControlCode == IO_GET_ENGINE_REQUEST)
	{
		PULONGLONG OutPut = (PULONGLONG)Irp->AssociatedIrp.SystemBuffer;
		*OutPut = EngineAddress;

		//DbgPrintEx(0, 0, "Engine Module get %#010x", EngineAddress);
		Status = STATUS_SUCCESS;
		BytesIO = sizeof(EngineAddress);
	}
	else if (ControlCode == IO_MOUSE_REQUEST)
	{
		PMOUSE_REQUEST ReadInput = (PMOUSE_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		mdata.Flags |= MOUSE_MOVE_RELATIVE;
		mdata.ButtonFlags &= ~MOUSE_LEFT_BUTTON_UP;
		mdata.ButtonFlags &= ~MOUSE_LEFT_BUTTON_DOWN;
		if (ReadInput->click && ReadInput->status == 1)
		{
			mdata.ButtonFlags |= MOUSE_LEFT_BUTTON_DOWN;
		}
		else if (ReadInput->click && ReadInput->status == 0)
		{
			mdata.ButtonFlags |= MOUSE_LEFT_BUTTON_UP;
		}
		else
		{
			DbgPrintEx(0, 0, "dx: %ld dy: %ld", ReadInput->dx, ReadInput->dy);
			mdata.LastX = ReadInput->dx;
			mdata.LastY = ReadInput->dy;
		}
		SynthesizeMouse(&mdata);

		Status = STATUS_SUCCESS;
		BytesIO = sizeof(PMOUSE_REQUEST);
	}
	else
	{
		 // if the code is unknown
		Status = STATUS_INVALID_PARAMETER;
		BytesIO = 0;
	}

	// Complete the request
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesIO;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return Status;
}

NTSTATUS InternalIoCtrl(PDEVICE_OBJECT device, PIRP irp)
{
	PIO_STACK_LOCATION ios;
	PCONNECT_DATA cd;

	ios = IoGetCurrentIrpStackLocation(irp);

	DbgPrintEx(0, 0, "InternalIoCtrl");
	if (ios->Parameters.DeviceIoControl.IoControlCode == MOUCLASS_CONNECT_REQUEST)
	{
		cd = ios->Parameters.DeviceIoControl.Type3InputBuffer;
		MouseDpcRoutine = (MouseServiceDpc)cd->ClassService;
		DbgPrintEx(0, 0, "MouseServiceDpc [%x]", MouseDpcRoutine);
	}
	else
	{
		return STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}

// Driver Entrypoint
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	UNICODE_STRING classNameBuffer;
	PFILE_OBJECT file;
	PDRIVER_OBJECT pMouseDriver;
	MouseAddDevice MouseAddDevicePtr;
	struct DEVOBJ_EXTENSION_FIX *DevObjExtension;
	ULONGLONG node = 0;
	SHORT *u;
	wchar_t mouname[22] = L"\\Device\\PointerClass0";

	DbgPrintEx(0, 0, "Driver Loaded\n");

	memset((void*)&mdata, 0, sizeof(mdata));
	memset((void*)MOU_DATA, 0, sizeof(MOU_DATA));

	PsSetLoadImageNotifyRoutine(ImageLoadCallback);

	RtlInitUnicodeString(&dev, L"\\Device\\Barbell");
	RtlInitUnicodeString(&dos, L"\\DosDevices\\Barbell");

	IoCreateDevice(pDriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	IoCreateSymbolicLink(&dos, &dev);

	RtlInitUnicodeString(&dev, L"\\Device\\BarbellMouse");
	IoCreateDevice(pDriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pMouseObject);
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;
	pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = InternalIoCtrl;
	pDriverObject->DriverUnload = UnloadDriver;

	pDeviceObject->Flags |= DO_DIRECT_IO;
	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	pMouseObject->Flags |= DO_BUFFERED_IO;
	pMouseObject->Flags &= DO_DEVICE_INITIALIZING;

	RtlInitUnicodeString(&classNameBuffer, mouname);
	u = mouname;

	DbgPrintEx(0, 0, "Driver Finding Mouse...\n");

	while (1)
	{
		//run till we run out of devices or find a devnode
		//FILE_READ_ATTRIBUTES
		if (My_IoGetDeviceObjectPointer(&classNameBuffer, FILE_READ_ATTRIBUTES, &file, &pMouseTarget))
			return STATUS_OBJECT_NAME_NOT_FOUND;
		ObDereferenceObject(file);
		node = FindDevNodeRecurse(pMouseTarget, &node);
		if (node) break;
		*(u + MOU_STRING_INC) += 1;
		mouId++;
		DbgPrintEx(0, 0, "Nope Mouse");
	}

	pMouseDriver = pMouseTarget->DriverObject;
	MouClassReadRoutine = (MouclassRead)pMouseDriver->MajorFunction[IRP_MJ_READ];
	pMouseDriver->MajorFunction[IRP_MJ_READ] = ReadMouse;
	DevObjExtension = pMouseObject->DeviceObjectExtension;
	DevObjExtension->DeviceNode = (void*)node;
	MouseAddDevicePtr = (MouseAddDevice)pMouseDriver->DriverExtension->AddDevice;
	MouseAddDevicePtr(pMouseDriver, pMouseObject);
	DbgPrintEx(0, 0, "Mouse Hooked");

	return STATUS_SUCCESS;
}



NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriverObject)
{
	DbgPrintEx(0, 0, "Unload routine called.\n");

	PsRemoveLoadImageNotifyRoutine(ImageLoadCallback);

	IoDeleteSymbolicLink(&dos);
	IoDeleteDevice(pDriverObject->DeviceObject);

	return STATUS_SUCCESS;
}

NTSTATUS CreateCall(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS CloseCall(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
