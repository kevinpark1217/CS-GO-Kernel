#pragma once
#include "common.h"

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