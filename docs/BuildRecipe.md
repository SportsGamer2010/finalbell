# Final Bell — Build Recipe (Windows)

## Required tools

1. **Unreal Engine 5.8** (installed): `C:\Program Files\Epic Games\UE_5.8`
2. **Visual Studio 2022** with Desktop development with C++ and the workloads/components Unreal 5.8 documents for Windows
3. **Git for Windows** + **Git LFS**
4. **Python 3.11+** (real install; not the Microsoft Store alias) for contract tests

## Game compile

```powershell
cd C:\Users\Shadow\Projects\final-bell\apps\game-unreal
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FinalBellEditor Win64 Development -Project="$PWD\FinalBell.uproject" -WaitMutex
```

Or: right-click `FinalBell.uproject` → Generate Visual Studio project files → open `.sln` → build Development Editor.

## Dedicated server (later packaging)

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" FinalBellServer Win64 Development -Project="$PWD\FinalBell.uproject" -WaitMutex
```

## Contract tests

```powershell
cd C:\Users\Shadow\Projects\final-bell\packages\domain-python
python -m pip install -e ".[dev]"
python -m pytest ..\..\tests\golden -q
```

## Two-client smoke (after map exists)

1. Host: `FinalBell.exe` or editor PIE listen server on port 7777  
2. Client: `open 127.0.0.1:7777`  
3. Record observed RTT/loss; confirm identical stamina/hit outcomes for the same intents  
4. Do **not** label LAN PIE as a verified internet service
