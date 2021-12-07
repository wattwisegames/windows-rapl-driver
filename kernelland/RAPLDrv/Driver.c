/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"
#include <intrin.h>
#include "msr-index.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, RAPLDrvEvtDeviceAdd)
#pragma alloc_text (PAGE, RAPLDrvEvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    PDEVICE_OBJECT device_object;
    UNICODE_STRING device_name;
    UNICODE_STRING sym_name;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = RAPLDrvEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
                           RAPLDrvEvtDeviceAdd
                           );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    /* Assign driver callbacks */
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;

    RtlInitUnicodeString(&device_name, L"\\Device\\RAPLDriver");
    RtlInitUnicodeString(&sym_name, DEVICE_NAME);
    status = IoCreateDevice(DriverObject, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
    if (NT_SUCCESS(status)) {
        IoCreateSymbolicLink(&sym_name, &device_name);
    }
    return status;
}

NTSTATUS
RAPLDrvEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    status = RAPLDrvCreateDevice(DeviceInit);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

VOID
RAPLDrvEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    //
    // Stop WPP Tracing
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
}

void DriverUnload(PDRIVER_OBJECT driver)
{
    UNICODE_STRING sym_name;

    RtlInitUnicodeString(&sym_name, DEVICE_NAME);
    IoDeleteSymbolicLink(&sym_name);
    IoDeleteDevice(driver->DeviceObject);
}

NTSTATUS DispatchCreate(PDEVICE_OBJECT device, PIRP irp)
{
    DbgPrint("Creating driver %s... \n", device->DriverObject->DriverName);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = STATUS_SUCCESS;
    IofCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(PDEVICE_OBJECT device, PIRP irp)
{
    DbgPrint("Closing driver %s... \n", device->DriverObject->DriverName);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = STATUS_SUCCESS;
    IofCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchCleanup(PDEVICE_OBJECT device, PIRP irp)
{
    DbgPrint("Cleanup driver %s... \n", device->DriverObject->DriverName);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = STATUS_SUCCESS;
    IofCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT device, PIRP irp)
{
    //PCHAR outBuffer;
    ULONG controlCode;
    NTSTATUS ntStatus;
    UINT16 functionCode;
    UINT32 msrRegister;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    ULONGLONG msrResult;
    LARGE_INTEGER currentTime;
    PIO_STACK_LOCATION stackLocation;

    stackLocation = irp->Tail.Overlay.CurrentStackLocation;
    controlCode = stackLocation->Parameters.DeviceIoControl.IoControlCode;
    functionCode = FunctionFromIOCTLCode(controlCode);
    inputBufferLength = stackLocation->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = stackLocation->Parameters.DeviceIoControl.OutputBufferLength;
    KeQuerySystemTime(&currentTime);

    DbgPrint("Received control code %u from %s.\n", controlCode, device->DriverObject->DriverName);
    DbgPrint("Received function code %u from %s.\n", functionCode, device->DriverObject->DriverName);

    /* METHOD_OUT_DIRECT */
    /*
    outBuffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority | MdlMappingNoExecute);
    if (!outBuffer)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        switch (functionCode)
        {
        case 0xBEB:
            msrResult = __readmsr(MSR_RAPL_POWER_UNIT);
            __writemsr(MSR_RAPL_POWER_UNIT, currentTime.QuadPart);
            break;

        case 0xBEC:
            msrResult = __readmsr(MSR_PKG_POWER_LIMIT);
            __writemsr(MSR_PKG_POWER_LIMIT, currentTime.QuadPart);
            break;

        case 0xBED:
            msrResult = __readmsr(MSR_PKG_ENERGY_STATUS);
            __writemsr(MSR_PKG_ENERGY_STATUS, currentTime.QuadPart);
            break;

        default:
            DbgPrint("Unknown function code 0x%04x, ignoring.\n", functionCode);
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        memcpy(outBuffer, &msrResult, sizeof(ULONGLONG));
        ntStatus = STATUS_SUCCESS;
        irp->IoStatus.Information = stackLocation->Parameters.DeviceIoControl.OutputBufferLength;
    }
    */

    /* METHOD_BUFFERED */
    if (inputBufferLength == sizeof(UINT32))
    {
        /* MSR register codes provided by userland must not exceed 4 bytes */
        memcpy(&msrRegister, irp->AssociatedIrp.SystemBuffer, sizeof(UINT32));
        switch (msrRegister)
        {
        case 1:
            msrResult = 0x1111111111111111;
            break;

        case 2:
            msrResult = 0x2222222222222222;
            break;

        case 3:
            msrResult = currentTime.QuadPart;
            break;

        default:
            msrResult = __readmsr(msrRegister);
            break;
        }
        memcpy(irp->AssociatedIrp.SystemBuffer, &msrResult, sizeof(ULONGLONG));
        ntStatus = STATUS_SUCCESS;
        irp->IoStatus.Information = sizeof(ULONGLONG);
    }
    else
    {
        DbgPrint("Bad input length provided. Expected %u bytes, got %u.\n", sizeof(UINT32), inputBufferLength);
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }

//end:
    irp->IoStatus.Status = ntStatus;
    IofCompleteRequest(irp, IO_NO_INCREMENT);

    return ntStatus;
}

UINT16 FunctionFromIOCTLCode(UINT32 code)
{
    return (code >> 2) & 0xFFF;
}