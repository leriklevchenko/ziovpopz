#include <shellapi.h>
#include <windows.h>

#include <cwchar>
#include <iterator>

namespace {

constexpr wchar_t kWindowClassName[] = L"TrayBackgroundAppWindowClass";
constexpr wchar_t kWindowTitle[] = L"Tray Background App";
constexpr wchar_t kMutexPrefix[] = L"Local\\TrayBackgroundAppSingleInstance_";
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT_PTR kTrayIconId = 1;
constexpr UINT kMenuFileExit = 1001;
constexpr UINT kTrayOpen = 2001;
constexpr UINT kTrayExit = 2002;

HINSTANCE g_instance = nullptr;
HWND g_mainWindow = nullptr;
HANDLE g_singleInstanceMutex = nullptr;
UINT g_taskbarCreatedMessage = 0;

bool IsHiddenStartup()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        return false;
    }

    bool hidden = false;
    for (int i = 1; i < argc; ++i) {
        if (std::wcscmp(argv[i], L"--hidden") == 0 ||
            std::wcscmp(argv[i], L"/hidden") == 0 ||
            std::wcscmp(argv[i], L"-hidden") == 0) {
            hidden = true;
            break;
        }
    }

    LocalFree(argv);
    return hidden;
}

void ShowMainWindow()
{
    ShowWindow(g_mainWindow, SW_SHOWNORMAL);
    SetForegroundWindow(g_mainWindow);
}

void RemoveTrayIcon()
{
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = g_mainWindow;
    data.uID = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &data);
}

void AddTrayIcon()
{
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = g_mainWindow;
    data.uID = kTrayIconId;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = kTrayCallbackMessage;
    data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(data.szTip, L"Tray Background App");

    Shell_NotifyIconW(NIM_ADD, &data);

    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);
}

void ExitApplication()
{
    RemoveTrayIcon();
    DestroyWindow(g_mainWindow);
}

void ShowTrayMenu()
{
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kTrayOpen, L"Открыть");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kTrayExit, L"Выход");

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(g_mainWindow);

    const UINT command = TrackPopupMenu(
        menu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
        cursor.x,
        cursor.y,
        0,
        g_mainWindow,
        nullptr);

    DestroyMenu(menu);

    switch (command) {
    case kTrayOpen:
        ShowMainWindow();
        break;
    case kTrayExit:
        ExitApplication();
        break;
    default:
        break;
    }
}

HMENU CreateMainMenu()
{
    HMENU menuBar = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();
    if (!menuBar || !fileMenu) {
        if (fileMenu) {
            DestroyMenu(fileMenu);
        }
        if (menuBar) {
            DestroyMenu(menuBar);
        }
        return nullptr;
    }

    AppendMenuW(fileMenu, MF_STRING, kMenuFileExit, L"Выход");
    AppendMenuW(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), L"Файл");
    return menuBar;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == g_taskbarCreatedMessage) {
        AddTrayIcon();
        return 0;
    }

    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == kMenuFileExit) {
            ExitApplication();
            return 0;
        }
        break;

    case kTrayCallbackMessage:
        switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
        case NIN_SELECT:
        case NIN_KEYSELECT:
            ShowMainWindow();
            return 0;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu();
            return 0;
        default:
            break;
        }
        break;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterMainWindowClass()
{
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = g_instance;
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    return RegisterClassExW(&windowClass) != 0;
}

bool CreateMainWindow()
{
    g_mainWindow = CreateWindowExW(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        420,
        nullptr,
        CreateMainMenu(),
        g_instance,
        nullptr);

    return g_mainWindow != nullptr;
}

bool AcquireSingleInstance()
{
    wchar_t userName[256]{};
    DWORD userNameLength = static_cast<DWORD>(std::size(userName));
    if (!GetUserNameW(userName, &userNameLength)) {
        wcscpy_s(userName, L"UnknownUser");
    }

    wchar_t mutexName[512]{};
    wcscpy_s(mutexName, kMutexPrefix);
    wcscat_s(mutexName, userName);

    g_singleInstanceMutex = CreateMutexW(nullptr, TRUE, mutexName);
    if (!g_singleInstanceMutex) {
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
        return false;
    }

    return true;
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    g_instance = instance;

    if (!AcquireSingleInstance()) {
        return 0;
    }

    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    if (!RegisterMainWindowClass() || !CreateMainWindow()) {
        if (g_singleInstanceMutex) {
            CloseHandle(g_singleInstanceMutex);
        }
        return 1;
    }

    AddTrayIcon();

    if (!IsHiddenStartup()) {
        ShowMainWindow();
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (g_singleInstanceMutex) {
        CloseHandle(g_singleInstanceMutex);
    }

    return static_cast<int>(message.wParam);
}
