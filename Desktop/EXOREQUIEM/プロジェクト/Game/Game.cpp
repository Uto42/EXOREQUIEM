/*****************************************************************//**
 * @file    Game.cpp
 * @brief   DirectX 11の基盤初期化、メインループ、およびシーンマネージャーの実行制御の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game.h"
#include "Game/Scene/TitleScene/TitleScene.h"
#include "Game/Scene/GameplayScene/GameplayScene.h"
#include "Game/Scene/LoadingScreen/GameLoadingScreen.h"
#include "Game/Systems/Sound/SoundManager.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
	: m_fullscreen(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
    m_sceneManager.reset();

    // COMやDirectXが生きているうちに、サウンドを安全に終了させる
    SoundManager::Instance().Finalize();

    // その後に COM の解放など
    CoUninitialize();
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();

    SoundManager::Instance().Initialize();

    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();

    CreateWindowSizeDependentResources();

    // 起動シーン設定
    m_sceneManager->SetScene<GameLoadingScreen>();

}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    SoundManager::Instance().Update();

    // キーボードステートトラッカーの更新
    m_keyboardTracker.Update(Keyboard::Get().GetState());

    // マウスステートトラッカーの更新
    m_mouseTracker.Update(Mouse::Get().GetState());

    // シーンの更新
    m_sceneManager->Update(elapsedTime);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    // TODO: Add your rendering code here.
    context;

    // シーンの描画
    m_sceneManager->Render();

    // fpsの表示
    std::wostringstream oss;
    oss << "fps:" << m_timer.GetFramesPerSecond();
    m_debugFont->AddString(oss.str().c_str(), SimpleMath::Vector2(0.0f, 0.0f));

    // デバッグ用文字列の描画
    m_debugFont->Render(m_states.get());

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::Black);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // ゲームに戻った時、FPS的な操作が必要ならRELATIVE、メニューならABSOLUTE
    DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
}

void Game::OnDeactivated()
{
    // 裏画面に行った時はマウスを解放する
    DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetdeltaTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    // デバイスリソース、またはスワップチェーンがまだない場合は何もしないで帰る
    if (!m_deviceResources || !m_deviceResources->GetSwapChain())
    {
        return;
    }

    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);

    //フルスクリーンか調べる
    BOOL fullscreen = FALSE;

    m_deviceResources->GetSwapChain()->GetFullscreenState(&fullscreen, nullptr);

    //フルスクリーンが解除されてしまったときの処理
    if (m_fullscreen != fullscreen)
    {
        m_fullscreen = fullscreen;

        //ResizeBuffers関数を呼び出す
        m_deviceResources->CreateWindowSizeDependentResources();
    }
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    //共通ステート
    m_states = std::make_unique<CommonStates>(device);
    
    // デバッグ用文字列表示オブジェクトの作成
    m_debugFont = std::make_unique<DebugFont>(device, context, L"Resources/Font/SegoeUI_18.spritefont");

    // ユーザーリソースの作成
    if (!m_userResources) m_userResources = std::make_unique<UserResources>();

    // シーンマネージャーの作成
    if (!m_sceneManager) m_sceneManager = std::make_unique<SceneManager<UserResources>>(m_userResources.get());

    // シーンへ渡すユーザーリソースの設定
    m_userResources->SetDeviceResources(m_deviceResources.get());
    m_userResources->SetCommonStates(m_states.get());
    m_userResources->SetDebugFont(m_debugFont.get());
    m_userResources->SetKeyboardStateTracker(&m_keyboardTracker);
    m_userResources->SetMouseStateTracker(&m_mouseTracker);
    m_userResources->SetStepTimerStates(&m_timer);

    // 実行中のシーンのCreateDeviceDependentResources関数を呼び出す
    m_sceneManager->CreateDeviceDependentResources();

    auto& sm = SoundManager::Instance();
    sm.LoadWave(L"BGM_Title", L"Resources/Sounds/BGM/bgm_title.wav");
    sm.LoadWave(L"BGM_Play", L"Resources/Sounds/BGM/bgm_gameplay.wav");
    sm.LoadWave(L"BGM_Standby", L"Resources/Sounds/BGM/bgm_standby.wav");
    sm.LoadWave(L"SE_Decision", L"Resources/Sounds/SE/se_ui_decision.wav");
    sm.LoadWave(L"SE_Explosion_Hit", L"Resources/Sounds/SE/se_explosion_hit.wav");
    sm.LoadWave(L"SE_Explosion_Defeat", L"Resources/Sounds/SE/se_explosion_defeat.wav");
    sm.LoadWave(L"SE_Missile", L"Resources/Sounds/SE/se_missile_fire.wav");
    sm.LoadWave(L"SE_Gun", L"Resources/Sounds/SE/se_gun_fire.wav");
    sm.LoadWave(L"SE_Booster", L"Resources/Sounds/SE/se_player_booster.wav");
    sm.LoadWave(L"SE_Ready", L"Resources/Sounds/SE/se_ready.wav");
    sm.LoadWave(L"SE_Go", L"Resources/Sounds/SE/se_go.wav");
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.

    // 実行中のシーンのCreateWindowSizeDependentResources関数を呼び出す
    m_sceneManager->CreateWindowSizeDependentResources();
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    // 実行中のシーンのOnDeviceLost関数を呼び出す
    m_sceneManager->OnDeviceLost();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

#pragma endregion