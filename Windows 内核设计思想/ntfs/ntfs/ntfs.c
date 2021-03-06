/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: ntfs.c 31747 2008-01-12 20:54:41Z pschweitzer $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/ntfs.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 *                   Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

PNTFS_GLOBAL_DATA NtfsGlobalData = NULL;


/* FUNCTIONS ****************************************************************/

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
    NTSTATUS Status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(DEVICE_NAME);
    TRACE_(NTFS, "DriverEntry(%p, '%wZ')\n", DriverObject, RegistryPath);
    /* Initialize global data */
    NtfsGlobalData = ExAllocatePoolWithTag(NonPagedPool, sizeof(NTFS_GLOBAL_DATA), TAG('N', 'D', 'R', 'G'));

    if (!NtfsGlobalData) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorEnd;
    }

    RtlZeroMemory(NtfsGlobalData, sizeof(NTFS_GLOBAL_DATA));
    NtfsGlobalData->Identifier.Type = NTFS_TYPE_GLOBAL_DATA;
    NtfsGlobalData->Identifier.Size = sizeof(NTFS_GLOBAL_DATA);
    ExInitializeResourceLite(&NtfsGlobalData->Resource);
    /* Keep trace of Driver Object */
    NtfsGlobalData->DriverObject = DriverObject;
    /* Initialize IRP functions array */
    NtfsInitializeFunctionPointers(DriverObject);
    /* Driver can't be unloaded */
    DriverObject->DriverUnload = NULL;
    Status = IoCreateDevice(DriverObject,
                            sizeof(NTFS_GLOBAL_DATA),
                            &DeviceName,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            0,
                            FALSE,
                            &NtfsGlobalData->DeviceObject);

    if (!NT_SUCCESS(Status)) {
        WARN_(NTFS, "IoCreateDevice failed with status: %lx\n", Status);
        goto ErrorEnd;
    }

    /* Register file system */
    IoRegisterFileSystem(NtfsGlobalData->DeviceObject);
    ObReferenceObject(NtfsGlobalData->DeviceObject);
ErrorEnd:

    if (!NT_SUCCESS(Status)) {
        if (NtfsGlobalData) {
            ExDeleteResourceLite(&NtfsGlobalData->Resource);
            ExFreePoolWithTag(NtfsGlobalData, TAG('N', 'D', 'R', 'G'));
        }
    }

    return Status;
}

VOID NTAPI
NtfsInitializeFunctionPointers(PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Called within the driver entry to initialize the IRP functions array
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 * RETURNS: Nothing
 */
{
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = NtfsClose;
    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = NtfsCreate;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = NtfsRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = NtfsWrite;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = NtfsFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = NtfsDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = NtfsQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = NtfsQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = NtfsSetVolumeInformation;
    return;
}

