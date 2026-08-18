//
// Main.cpp
//

#include "pch.h"
#include "Game/Game.h"

using namespace DirectX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

// ウインドウスタイル
#define WS_MYWINDOW (WS_OVERLAPPED  | \
                     WS_CAPTION     | \
                     WS_SYSMENU     | \
                     WS_MINIMIZEBOX)

namespace
{
    std::unique_ptr<Game> g_game;
}

LPCWSTR g_szAppName = L"EXO REQUIEM";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ExitGame() noexcept;

// ハイブリッドグラフィックス（ノートPC等の内蔵GPUと外部GPUの切り替え）システムにおいて、
// デフォルトで高性能な外部GPU（NVIDIA/AMD）を使用するように指示する設定
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// エントリーポイント（プログラムの開始点）
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!XMVerifyCPUSupport())
        return 1;

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (FAILED(hr))
        return 1;

    // キーボードの作成
    std::unique_ptr<Keyboard> keyboard = std::make_unique<Keyboard>();

    // マウスの作成
    std::unique_ptr<Mouse> mouse = std::make_unique<Mouse>();

    g_game = std::make_unique<Game>();

    // フルスクリーン

    static bool s_fullscreen = false;

    //画面モード選択
    int result = MessageBox(NULL, L"フルスクリーンにしますか？", L"画面モード設定", MB_YESNO);

    //?ボタンが押された、または「キャンセル」が押された場合はプログラム自体を終了
    if (result == IDCANCEL)
    {
        return 0;
    }
    //フルスクリーン
    else if (result == IDYES)
    {
        s_fullscreen = true;
    }
    else
    {
        s_fullscreen = false;
    }

    // ウィンドウクラスの登録とウィンドウの生成
    {
        // ウィンドウの設定（クラスの登録）
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = L"_EXOREQUIEM";
        wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
        if (!RegisterClassExW(&wcex))
            return 1;

        // ウィンドウの作成
        int w, h;
        g_game->GetDefaultSize(w, h);

        RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

        AdjustWindowRect(&rc, WS_MYWINDOW, FALSE);

        HWND hwnd = CreateWindowExW(0, L"_EXOREQUIEM", g_szAppName, WS_MYWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInstance,
            g_game.get());

        if (!hwnd)
            return 1;

        // マウスにウインドウハンドルを渡す
        mouse->SetWindow(hwnd);

        ShowWindow(hwnd, nCmdShow);

        GetClientRect(hwnd, &rc);

        g_game->Initialize(hwnd, rc.right - rc.left, rc.bottom - rc.top);

        //フルスクリーンに変更
        if (s_fullscreen)
        {
            // ウィンドウの枠とタイトルバーを消す
            SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
            // モニターの解像度を取得して、画面いっぱいに広げる
            SetWindowPos(hwnd, HWND_TOP, 0, 0,
                GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                SWP_FRAMECHANGED);
            ShowWindow(hwnd, SW_SHOW);
        }
    }

    // メインメッセージループ
    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_game->Tick();
        }
    }

    //// フルスクリーン

    //if (s_fullscreen)
    //{
    //    g_game->SetFullscreenState(FALSE);
    //}

    g_game.reset();

    CoUninitialize();

    return static_cast<int>(msg.wParam);
}

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool s_in_sizemove = false;
    static bool s_in_suspend = false;
    static bool s_minimized = false;
    static bool s_fullscreen = false;

    auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        if (lParam)
        {
            auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
        }
        break;

    case WM_PAINT:
        if (s_in_sizemove && game)
        {
            game->Tick();
        }
        else
        {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DISPLAYCHANGE:
        if (game)
        {
            game->OnDisplayChange();
        }
        break;

    case WM_MOVE:
        if (game)
        {
            game->OnWindowMoved();
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            if (!s_minimized)
            {
                s_minimized = true;
                if (!s_in_suspend && game)
                    game->OnSuspending();
                s_in_suspend = true;
            }
        }
        else if (s_minimized)
        {
            s_minimized = false;
            if (s_in_suspend && game)
                game->OnResuming();
            s_in_suspend = false;
        }
        else if (!s_in_sizemove && game)
        {
            game->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_ENTERSIZEMOVE:
        s_in_sizemove = true;
        break;

    case WM_EXITSIZEMOVE:
        s_in_sizemove = false;
        if (game)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);

            game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top);
        }
        break;

    case WM_GETMINMAXINFO:
        if (lParam)
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 320;
            info->ptMinTrackSize.y = 200;
        }
        break;

        //右上の?が押されたとき
    case WM_CLOSE:

        DestroyWindow(hWnd);
        return 0;

        //ウィンドウが破棄されるときの処理
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_ACTIVATEAPP:
        
        Keyboard::ProcessMessage(message, wParam, lParam);
        Mouse::ProcessMessage(message, wParam, lParam);

        if (game)
        {
            if (wParam)
            {
                game->OnActivated();
            }
            else
            {
                game->OnDeactivated();
            }
        }
        break;

    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMQUERYSUSPEND:
            if (!s_in_suspend && game)
                game->OnSuspending();
            s_in_suspend = true;
            return TRUE;

        case PBT_APMRESUMESUSPEND:
            if (!s_minimized)
            {
                if (s_in_suspend && game)
                    game->OnResuming();
                s_in_suspend = false;
            }
            return TRUE;
        }
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
        {
            // Alt + Enter によるフルスクリーン切り替えの処理
            if (s_fullscreen)
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

                int width = 1280;
                int height = 720;
                if (game)
                    game->GetDefaultSize(width, height);

                // 枠の分を含めた正しいウィンドウサイズを再計算
                RECT rc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
                AdjustWindowRect(&rc, WS_MYWINDOW, FALSE);

                ShowWindow(hWnd, SW_SHOWNORMAL);
                SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0,
                    rc.right - rc.left, rc.bottom - rc.top,
                    SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
            else
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

                // モニターの解像度を取得して広げる
                SetWindowPos(hWnd, HWND_TOP, 0, 0,
                    GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                    SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

                ShowWindow(hWnd, SW_SHOW);
            }

            s_fullscreen = !s_fullscreen;
        }
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;

    case WM_MENUCHAR:
        // メニュー有効時のビープ音を抑制
        return MAKELRESULT(0, MNC_CLOSE);

    case WM_ACTIVATE:
//    case WM_ACTIVATEAPP:
        Keyboard::ProcessMessage(message, wParam, lParam);
        Mouse::ProcessMessage(message, wParam, lParam);
        break;

    //case WM_SYSKEYDOWN:
    //    if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
    //    {
    //        // This is where you'd implement the classic ALT+ENTER hotkey for fullscreen toggle
    //        ...
    //    }
    //    Keyboard::ProcessMessage(message, wParam, lParam);
    //    break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;

//    case WM_ACTIVATE:
//    case WM_ACTIVATEAPP:
    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        Mouse::ProcessMessage(message, wParam, lParam);
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ゲーム終了ヘルパー
void ExitGame() noexcept
{
    PostQuitMessage(0);
}
