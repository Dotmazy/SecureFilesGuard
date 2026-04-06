/*
 * FolderGuardCommon.h
 * Structures partagées entre le driver kernel et l'application userland.
 * Inclus dans les deux projets Visual Studio.
 */

#pragma once

 /* Nom du port de communication kernel ↔ userland */
#define FG_PORT_NAME    L"\\FolderGuardPort"

/* ─── Types d'accès ──────────────────────────────────────────────────────── */

typedef enum _FG_ACCESS_TYPE {
    FG_ACCESS_READ = 0,
    FG_ACCESS_WRITE = 1,
} FG_ACCESS_TYPE;

/* ─── Décision de l'utilisateur ──────────────────────────────────────────── */

typedef enum _FG_DECISION {
    FG_DECISION_ALLOW = 0,
    FG_DECISION_DENY = 1,
} FG_DECISION;

/* ─── Message kernel → userland : notification d'accès ──────────────────── */

typedef struct _FG_NOTIFICATION {
    ULONG           RequestId;              // ID unique pour matcher la réponse
    ULONG           ProcessId;             // PID du processus demandeur
    FG_ACCESS_TYPE  AccessType;            // Lecture ou écriture
    WCHAR           ProcessName[64];       // Nom de l'exe (ex: notepad.exe)
    WCHAR           FilePath[512];         // Chemin complet du fichier
} FG_NOTIFICATION, * PFG_NOTIFICATION;

/* ─── Message userland → kernel ──────────────────────────────────────────── */

typedef enum _FG_MSG_TYPE {
    FG_MSG_RESPONSE = 0,   // Réponse YES/NO à une notification
    FG_MSG_SET_DIR = 1,   // Définir le dossier protégé
} FG_MSG_TYPE;

typedef struct _FG_USER_MESSAGE {
    FG_MSG_TYPE Type;
    ULONG       RequestId;      // Pour FG_MSG_RESPONSE : ID de la notif
    FG_DECISION Decision;       // Pour FG_MSG_RESPONSE : ALLOW ou DENY
    WCHAR       DirPath[512];   // Pour FG_MSG_SET_DIR  : chemin device path
} FG_USER_MESSAGE, * PFG_USER_MESSAGE;
