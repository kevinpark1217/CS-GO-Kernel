#include <ntdef.h>
#include <ntifs.h>
#include <ntddk.h>
#include "Memory.h"

// Request to read virtual user memory (memory of a program) from kernel space
#define IO_READ_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5851 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to write virtual user memory (memory of a program) from kernel space
//#define IO_WRITE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5852 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to retrieve the base address of client.dll in csgo.exe from kernel space
#define IO_GET_CLIENT_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5854 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Request to retrieve the base address of engine.dll in csgo.exe from kernel space
#define IO_GET_ENGINE_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 5855 /* Our Custom Code */, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

PDEVICE_OBJECT pDeviceObject; // our driver object
UNICODE_STRING dev, dos; // Driver registry paths

HANDLE ProcessHandle;
ULONGLONG ClientAddress, EngineAddress;

// datatype for read request
typedef struct _KERNEL_READ_REQUEST
{
	ULONGLONG Address;
	ULONGLONG Response;
	SIZE_T Size;

} KERNEL_READ_REQUEST, *PKERNEL_READ_REQUEST;

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

// Driver Entrypoint
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	//DbgPrintEx(0, 0, "Driver Loaded\n");

	PsSetLoadImageNotifyRoutine(ImageLoadCallback);

	RtlInitUnicodeString(&dev, L"\\Device\\Barbell");
	RtlInitUnicodeString(&dos, L"\\DosDevices\\Barbell");

	IoCreateDevice(pDriverObject, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	IoCreateSymbolicLink(&dos, &dev);

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;
	pDriverObject->DriverUnload = UnloadDriver;

	pDeviceObject->Flags |= DO_DIRECT_IO;
	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}



NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriverObject)
{
	//DbgPrintEx(0, 0, "Unload routine called.\n");

	PsRemoveLoadImageNotifyRoutine(ImageLoadCallback);
	IoDeleteSymbolicLink(&dos);
	IoDeleteDevice(pDriverObject->DeviceObject);
	//return STATUS_SUCCESS;
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
