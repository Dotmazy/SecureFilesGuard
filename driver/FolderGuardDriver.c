/*
 * FolderGuardDriver.c — Minifilter FolderGuard
 * Compile : cl /kernel /Gz /W3 /WX- /O2 /GS- /Zi /D NDEBUG /D _WIN64
 *           /I "F:\Windows Kits\10\Include\10.0.26100.0\km"
 *           /I "F:\Windows Kits\10\Include\10.0.26100.0\shared"
 *           /Fo FolderGuardDriver.obj FolderGuardDriver.c
 * Link   :  link /SUBSYSTEM:NATIVE /ENTRY:DriverEntry /DRIVER /MACHINE:X64
 *           /LIBPATH:"F:\Windows Kits\10\Lib\10.0.26100.0\km\x64"
 *           fltmgr.lib ntoskrnl.lib /OUT:FolderGuardDriver.sys FolderGuardDriver.obj
 */

 /* ── Macros SAL manquantes sur certaines versions WDK ─────────────────────── */
#ifndef _Flt_CompletionContext_Outptr_
#define _Flt_CompletionContext_Outptr_  _Outptr_result_maybenull_
#endif
#ifndef _Flt_ConnectionCookie_OutPtr_
#define _Flt_ConnectionCookie_OutPtr_   _Outptr_result_maybenull_
#endif

#define _X86_

#include <fltKernel.h>
#include <dontuse.h>
#include "FolderGuardCommon.h"

/* PsGetProcessImageFileName : pas toujours exposé via fltKernel.h seul */
NTKERNELAPI PUCHAR PsGetProcessImageFileName(_In_ PEPROCESS Process);

/* ExAllocatePool2 dispo depuis Win10 2004 (NTDDI_WIN10_VB = 0x0A000008) */
#if (NTDDI_VERSION >= 0x0A000008)
#  define FG_ALLOC(sz, tag)  ExAllocatePool2(POOL_FLAG_NON_PAGED, (sz), (tag))
#else
#  pragma warning(suppress: 4996)
#  define FG_ALLOC(sz, tag)  ExAllocatePoolWithTag(NonPagedPool, (sz), (tag))
#endif

/* ── Prototypes ────────────────────────────────────────────────────────────── */

DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);

NTSTATUS FgUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

NTSTATUS FgInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS       FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS    Flags,
    _In_ DEVICE_TYPE                 VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE         VolumeFilesystemType);

NTSTATUS FgInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS              FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS  Flags);

FLT_PREOP_CALLBACK_STATUS FgPreCreate(
    _Inout_ PFLT_CALLBACK_DATA        Data,
    _In_    PCFLT_RELATED_OBJECTS     FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext);

FLT_PREOP_CALLBACK_STATUS FgPreRead(
    _Inout_ PFLT_CALLBACK_DATA        Data,
    _In_    PCFLT_RELATED_OBJECTS     FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext);

NTSTATUS FgConnectNotify(
    _In_     PFLT_PORT  ClientPort,
    _In_opt_ PVOID      ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_     ULONG      SizeOfContext,
    _Outptr_result_maybenull_ PVOID* ConnectionCookie);

VOID FgDisconnectNotify(_In_opt_ PVOID ConnectionCookie);

NTSTATUS FgMessageNotify(
    _In_opt_ PVOID  PortCookie,
    _In_reads_bytes_opt_(InputBufferLength)  PVOID InputBuffer,
    _In_     ULONG  InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_     ULONG  OutputBufferLength,
    _Out_    PULONG ReturnOutputBufferLength);

/* ── Données globales ──────────────────────────────────────────────────────── */

PFLT_FILTER     g_FilterHandle = NULL;
PFLT_PORT       g_ServerPort = NULL;
PFLT_PORT       g_ClientPort = NULL;
UNICODE_STRING  g_ProtectedDir = { 0 };
WCHAR           g_ProtectedDirBuf[512] = { 0 };

/* ── Table des requêtes en attente ─────────────────────────────────────────── */

#define MAX_PENDING 32

typedef struct _PENDING_REQUEST {
    ULONG           RequestId;
    PFLT_CALLBACK_DATA Data;
    BOOLEAN         InUse;
    KEVENT          ResponseEvent;
    FG_DECISION     Decision;
} PENDING_REQUEST;

PENDING_REQUEST g_Pending[MAX_PENDING];

/*
 * On utilise EX_SPIN_LOCK (plus simple sur x64) au lieu de KSPIN_LOCK.
 * ExAcquireSpinLockExclusive / ExReleaseSpinLockExclusive sont de vraies
 * fonctions exportées par ntoskrnl.lib → pas de problème de symboles.
 */
EX_SPIN_LOCK    g_PendingLock = 0;
ULONG           g_NextRequestId = 1;

/* ── Callbacks enregistrés ─────────────────────────────────────────────────── */

CONST FLT_OPERATION_REGISTRATION g_Callbacks[] = {
    { IRP_MJ_CREATE, 0, FgPreCreate, NULL },
    { IRP_MJ_READ,   0, FgPreRead,   NULL },
    { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION g_FilterRegistration = {
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    0,
    NULL,
    g_Callbacks,
    FgUnload,
    FgInstanceSetup,
    FgInstanceQueryTeardown,
    NULL, NULL, NULL, NULL, NULL, NULL
};

/* ── DriverEntry ───────────────────────────────────────────────────────────── */

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING portName;
    PSECURITY_DESCRIPTOR sd = NULL;

    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("[FolderGuard] DriverEntry\n");

    RtlZeroMemory(g_Pending, sizeof(g_Pending));

    status = FltRegisterFilter(DriverObject, &g_FilterRegistration, &g_FilterHandle);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[FolderGuard] FltRegisterFilter failed: 0x%X\n", status);
        return status;
    }

    /* Security Descriptor : autorise tout Admin a se connecter au port */
    status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[FolderGuard] FltBuildDefaultSecurityDescriptor failed: 0x%X\n", status);
        FltUnregisterFilter(g_FilterHandle);
        return status;
    }

    RtlInitUnicodeString(&portName, FG_PORT_NAME);
    InitializeObjectAttributes(&oa, &portName,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);

    status = FltCreateCommunicationPort(
        g_FilterHandle, &g_ServerPort, &oa, NULL,
        FgConnectNotify, FgDisconnectNotify, FgMessageNotify, 1);

    FltFreeSecurityDescriptor(sd);

    if (!NT_SUCCESS(status)) {
        DbgPrint("[FolderGuard] FltCreateCommunicationPort failed: 0x%X\n", status);
        FltUnregisterFilter(g_FilterHandle);
        return status;
    }

    status = FltStartFiltering(g_FilterHandle);
    if (!NT_SUCCESS(status)) {
        DbgPrint("[FolderGuard] FltStartFiltering failed: 0x%X\n", status);
        FltCloseCommunicationPort(g_ServerPort);
        FltUnregisterFilter(g_FilterHandle);
        return status;
    }

    DbgPrint("[FolderGuard] Started OK\n");
    return STATUS_SUCCESS;
}

/* ── Déchargement ──────────────────────────────────────────────────────────── */

NTSTATUS FgUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);
    FltCloseCommunicationPort(g_ServerPort);
    FltUnregisterFilter(g_FilterHandle);
    DbgPrint("[FolderGuard] Unloaded\n");
    return STATUS_SUCCESS;
}

NTSTATUS FgInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    if (VolumeFilesystemType == FLT_FSTYPE_RAW) return STATUS_FLT_DO_NOT_ATTACH;
    return STATUS_SUCCESS;
}

NTSTATUS FgInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    return STATUS_SUCCESS;
}

/* ── Helpers ───────────────────────────────────────────────────────────────── */

static BOOLEAN IsInProtectedDir(_In_ PUNICODE_STRING FilePath)
{
    UNICODE_STRING prefix;
    if (g_ProtectedDir.Length == 0) return FALSE;
    if (FilePath->Length < g_ProtectedDir.Length) return FALSE;
    prefix.Buffer = g_ProtectedDir.Buffer;
    prefix.Length = g_ProtectedDir.Length;
    prefix.MaximumLength = g_ProtectedDir.Length;
    return RtlPrefixUnicodeString(&prefix, FilePath, TRUE);
}

static NTSTATUS GetFilePath(
    _In_  PFLT_CALLBACK_DATA   Data,
    _Out_ PUNICODE_STRING      FilePath,
    _Out_ PWCH* OutBuffer)
{
    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
    ULONG len;

    *OutBuffer = NULL;
    status = FltGetFileNameInformation(Data,
        FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
    if (!NT_SUCCESS(status)) return status;

    status = FltParseFileNameInformation(nameInfo);
    if (!NT_SUCCESS(status)) { FltReleaseFileNameInformation(nameInfo); return status; }

    len = nameInfo->Name.Length + sizeof(WCHAR);
    *OutBuffer = FG_ALLOC(len, 'FGPN');
    if (!*OutBuffer) {
        FltReleaseFileNameInformation(nameInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(*OutBuffer, nameInfo->Name.Buffer, nameInfo->Name.Length);
    (*OutBuffer)[nameInfo->Name.Length / sizeof(WCHAR)] = L'\0';
    FilePath->Buffer = *OutBuffer;
    FilePath->Length = nameInfo->Name.Length;
    FilePath->MaximumLength = (USHORT)len;

    FltReleaseFileNameInformation(nameInfo);
    return STATUS_SUCCESS;
}

static void GetProcessName(_Out_writes_(64) PWCHAR OutName)
{
    PEPROCESS proc = PsGetCurrentProcess();
    PUCHAR name = PsGetProcessImageFileName(proc);
    ULONG i;
    if (!name) { OutName[0] = L'\0'; return; }
    for (i = 0; i < 63 && name[i]; i++) OutName[i] = (WCHAR)name[i];
    OutName[i] = L'\0';
}

/* ── Gestion des slots pending (EX_SPIN_LOCK, sans KfAcquireSpinLock) ──────── */

static INT AllocPendingSlot(_In_ PFLT_CALLBACK_DATA Data, _Out_ ULONG* OutId)
{
    KIRQL irql;
    INT found = -1;
    INT i;

    irql = ExAcquireSpinLockExclusive(&g_PendingLock);
    for (i = 0; i < MAX_PENDING; i++) {
        if (!g_Pending[i].InUse) {
            g_Pending[i].InUse = TRUE;
            g_Pending[i].Data = Data;
            g_Pending[i].RequestId = g_NextRequestId++;
            g_Pending[i].Decision = FG_DECISION_DENY;
            KeInitializeEvent(&g_Pending[i].ResponseEvent, NotificationEvent, FALSE);
            *OutId = g_Pending[i].RequestId;
            found = i;
            break;
        }
    }
    ExReleaseSpinLockExclusive(&g_PendingLock, irql);
    return found;
}

static void FreePendingSlot(INT idx)
{
    KIRQL irql;
    irql = ExAcquireSpinLockExclusive(&g_PendingLock);
    g_Pending[idx].InUse = FALSE;
    ExReleaseSpinLockExclusive(&g_PendingLock, irql);
}

/* ── Cœur : intercepter et suspendre l'IRP ─────────────────────────────────── */

static FLT_PREOP_CALLBACK_STATUS AskUserland(
    _Inout_ PFLT_CALLBACK_DATA    Data,
    _In_    PCFLT_RELATED_OBJECTS FltObjects,
    _In_    FG_ACCESS_TYPE        AccessType)
{
    UNICODE_STRING  filePath;
    PWCH            filePathBuf = NULL;
    NTSTATUS        status;
    WCHAR           procName[64];
    FG_NOTIFICATION notif;
    ULONG           reqId, replyLen = 0;
    INT             slot;
    LARGE_INTEGER   timeout;
    FG_DECISION     decision;

    UNREFERENCED_PARAMETER(FltObjects);

    if (g_ClientPort == NULL) return FLT_PREOP_SUCCESS_NO_CALLBACK;

    status = GetFilePath(Data, &filePath, &filePathBuf);
    if (!NT_SUCCESS(status)) return FLT_PREOP_SUCCESS_NO_CALLBACK;

    if (!IsInProtectedDir(&filePath)) {
        ExFreePoolWithTag(filePathBuf, 'FGPN');
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    GetProcessName(procName);

    RtlZeroMemory(&notif, sizeof(notif));
    notif.AccessType = AccessType;
    notif.ProcessId = (ULONG)(ULONG_PTR)PsGetCurrentProcessId();
    RtlCopyMemory(notif.ProcessName, procName,
        min(sizeof(notif.ProcessName) - sizeof(WCHAR), 63 * sizeof(WCHAR)));
    RtlCopyMemory(notif.FilePath, filePath.Buffer,
        min(filePath.Length, sizeof(notif.FilePath) - sizeof(WCHAR)));

    ExFreePoolWithTag(filePathBuf, 'FGPN');

    slot = AllocPendingSlot(Data, &reqId);
    if (slot < 0) return FLT_PREOP_SUCCESS_NO_CALLBACK;
    notif.RequestId = reqId;

    status = FltSendMessage(g_FilterHandle, &g_ClientPort,
        &notif, sizeof(notif), NULL, &replyLen, NULL);
    if (!NT_SUCCESS(status)) {
        FreePendingSlot(slot);
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    /* Attendre la décision de l'utilisateur — max 30 secondes */
    timeout.QuadPart = -300000000LL;
    status = KeWaitForSingleObject(&g_Pending[slot].ResponseEvent,
        Executive, KernelMode, FALSE, &timeout);

    decision = g_Pending[slot].Decision;
    FreePendingSlot(slot);

    if (status == STATUS_TIMEOUT || decision == FG_DECISION_DENY) {
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;
        return FLT_PREOP_COMPLETE;
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

/* ── Callbacks ─────────────────────────────────────────────────────────────── */

FLT_PREOP_CALLBACK_STATUS FgPreCreate(
    _Inout_ PFLT_CALLBACK_DATA       Data,
    _In_    PCFLT_RELATED_OBJECTS    FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext)
{
    ACCESS_MASK access;
    FG_ACCESS_TYPE type;

    UNREFERENCED_PARAMETER(CompletionContext);

    if (FltGetRequestorProcessId(Data) == 4) return FLT_PREOP_SUCCESS_NO_CALLBACK;
    if (Data->RequestorMode == KernelMode)   return FLT_PREOP_SUCCESS_NO_CALLBACK;

    access = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
    type = (access & (FILE_READ_DATA | FILE_EXECUTE | GENERIC_READ | GENERIC_ALL))
        ? FG_ACCESS_READ : FG_ACCESS_WRITE;

    return AskUserland(Data, FltObjects, type);
}

FLT_PREOP_CALLBACK_STATUS FgPreRead(
    _Inout_ PFLT_CALLBACK_DATA       Data,
    _In_    PCFLT_RELATED_OBJECTS    FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext)
{
    UNREFERENCED_PARAMETER(CompletionContext);
    if (Data->RequestorMode == KernelMode) return FLT_PREOP_SUCCESS_NO_CALLBACK;
    return AskUserland(Data, FltObjects, FG_ACCESS_READ);
}

/* ── Port de communication kernel ↔ userland ───────────────────────────────── */

NTSTATUS FgConnectNotify(
    _In_     PFLT_PORT  ClientPort,
    _In_opt_ PVOID      ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_     ULONG      SizeOfContext,
    _Outptr_result_maybenull_ PVOID* ConnectionCookie)
{
    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionCookie);
    g_ClientPort = ClientPort;
    DbgPrint("[FolderGuard] Userland app connected\n");
    return STATUS_SUCCESS;
}

VOID FgDisconnectNotify(_In_opt_ PVOID ConnectionCookie)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);
    FltCloseClientPort(g_FilterHandle, &g_ClientPort);
    g_ClientPort = NULL;
    DbgPrint("[FolderGuard] Userland app disconnected\n");
}

NTSTATUS FgMessageNotify(
    _In_opt_ PVOID  PortCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_     ULONG  InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_     ULONG  OutputBufferLength,
    _Out_    PULONG ReturnOutputBufferLength)
{
    PFG_USER_MESSAGE msg;
    ULONG len;
    KIRQL irql;
    INT i;

    UNREFERENCED_PARAMETER(PortCookie);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    *ReturnOutputBufferLength = 0;

    if (!InputBuffer || InputBufferLength < sizeof(FG_USER_MESSAGE))
        return STATUS_INVALID_PARAMETER;

    msg = (PFG_USER_MESSAGE)InputBuffer;

    if (msg->Type == FG_MSG_SET_DIR) {
        len = (ULONG)(wcslen(msg->DirPath) * sizeof(WCHAR));
        if (len > sizeof(g_ProtectedDirBuf) - sizeof(WCHAR))
            len = sizeof(g_ProtectedDirBuf) - sizeof(WCHAR);
        RtlCopyMemory(g_ProtectedDirBuf, msg->DirPath, len);
        g_ProtectedDirBuf[len / sizeof(WCHAR)] = L'\0';
        g_ProtectedDir.Buffer = g_ProtectedDirBuf;
        g_ProtectedDir.Length = (USHORT)len;
        g_ProtectedDir.MaximumLength = sizeof(g_ProtectedDirBuf);
        DbgPrint("[FolderGuard] Protected dir: %ws\n", g_ProtectedDirBuf);
    }
    else if (msg->Type == FG_MSG_RESPONSE) {
        irql = ExAcquireSpinLockExclusive(&g_PendingLock);
        for (i = 0; i < MAX_PENDING; i++) {
            if (g_Pending[i].InUse && g_Pending[i].RequestId == msg->RequestId) {
                g_Pending[i].Decision = msg->Decision;
                KeSetEvent(&g_Pending[i].ResponseEvent, 0, FALSE);
                break;
            }
        }
        ExReleaseSpinLockExclusive(&g_PendingLock, irql);
    }

    return STATUS_SUCCESS;
}
