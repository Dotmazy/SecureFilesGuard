# TECH_INFO.md

## Overview

This document provides detailed technical information about building and running the project, including manual steps required for the driver build process due to encountered issues.

---

## Prerequisites

### Software

* Windows 10/11 (x64)
* Visual Studio (recommended: 2026)

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

### 1. Build Userland / Main Project

Using Visual Studio:

1. Open the solution file (.sln)
2. Select configuration:

   * Release / Debug
   * x64
3. Build the solution:

```
Build -> Build Solution
```

Or via command line:

```bash
msbuild project.sln /p:Configuration=Release /p:Platform=x64
```

---

## Driver Build (Important)

⚠️ Due to build issues, the driver **cannot be built fully automatically**.

Instead, follow these steps:

### Step 1 — Compile to .obj (Visual Studio)

1. Open the driver project in Visual Studio
2. Build ONLY the compilation step
3. Retrieve the generated `.obj` file

Typical output path:

```
/driver/x64/Release/*.obj
```

---

### Step 2 — Manual Linking to .sys

Use the **Developer Command Prompt for VS**.

Navigate to the directory containing the `.obj` file:

```bash
cd path\to\obj
```

Then manually link the driver:

```bash
link /SUBSYSTEM:NATIVE /DRIVER /OUT:driver.sys your_driver.obj
```

Depending on your setup, you may need additional flags:

```bash
link /SUBSYSTEM:NATIVE /DRIVER /ENTRY:DriverEntry /OUT:driver.sys your_driver.obj ntoskrnl.lib
```

---

## Notes on Driver Issues

* Visual Studio may fail during full driver build
* The workaround is:

  * Compile → `.obj`
  * Link manually → `.sys`
* Ensure correct architecture (x64)
* Ensure WDK libraries are accessible

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
sc create MyDriver type= kernel binPath= C:\path\to\driver.sys
sc start MyDriver
```

---

### Unload the Driver

```bash
sc stop MyDriver
sc delete MyDriver
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
