# TrayBackgroundApp

Native Win32 API tray application for Windows.

## Features

- Adds an icon to the taskbar notification area on startup.
- Left click on the tray icon opens the main window.
- Right click on the tray icon opens a context menu with `Открыть` and `Выход`.
- Restores the tray icon after taskbar recreation.
- Supports hidden startup with `--hidden`, `/hidden`, or `-hidden`.
- Keeps running in the background when the main window is closed.
- Main window menu contains `Файл -> Выход`.
- Prevents multiple instances for the same user using a named mutex.

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable is produced at:

```text
build\Release\TrayBackgroundApp.exe
```

## Run Hidden

```powershell
.\build\Release\TrayBackgroundApp.exe --hidden
```
