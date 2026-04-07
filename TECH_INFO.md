# TECH_INFO.md

## Overview

This document provides detailed technical information about building and running the project, including manual steps required for the driver build process due to encountered issues.

---

## Prerequisites

### Software

* Windows 10/11 (x64)
* Visual Studio (recommended: 2022)

  * Workloads:

    * Desktop development with C++
    * Windows Driver Kit (WDK)
* Windows SDK
* WDK (matching your Windows version)
* Developer Command Prompt (from Visual Studio)

### Optional

* Administrator privileges (required for driver loading)

---

## Project Structure

```
/project-root
 ├── src/
 ├── driver/
 ├── build/
 ├── scripts/
 └── README.md
```

---

## Build Instructions

Change the path of winkits in `build.bat`:
```bat
: Path of windows kits
set WINKITPATH=F:\Windows Kits\10
```

Run `build.bat`

---

## Running the Driver

### Enable Test Signing Mode

```bash
bcdedit /set testsigning on
```

Reboot required.

---

### Load the Driver

Using `sc`:

```bash
sc create FolderGuardDriver type= kernel binPath= C:\path\to\driver.sys
sc start FolderGuardDriver
```

---

### Unload the Driver

```bash
sc stop FolderGuardDriver
sc delete FolderGuardDriver
```

---

## Debugging

### Common Issues

* Driver not loading → Signature enforcement
* Missing symbols → Incorrect link flags
* BSOD → Kernel bug (use WinDbg)

### Recommended Tools

* WinDbg
* DebugView
* Device Manager (for driver status)

---

## Build Tips

* Always build in x64
* Keep paths short (avoid Windows path length issues)
* Run tools as Administrator when needed
* Clean build if weird errors appear:

```bash
msbuild /t:Clean
```

---

## Summary

Because of build issues:

1. Compile driver → `.obj` (Visual Studio)
2. Link manually → `.sys` (command line)
3. Load using `sc`

This hybrid workflow is currently required for a successful build.

---

## TODO

* Fix full driver build pipeline
* Automate `.sys` generation
* Add signing process

---
