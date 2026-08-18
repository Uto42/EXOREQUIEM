/*****************************************************************//**
 * @file    Game.h
 * @brief   DirectX 11の基盤初期化、メインループ、およびシーンマネージャーの実行制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Common/DeviceResources.h"
#include "Game/Common/StepTimer.h"
#include "Scene/SceneManager.h"
#include "Game/Common/UserResources.h"


class Game final : public DX::IDeviceNotify
{
public:
    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // --- 初期化と管理 ---
    void Initialize(HWND window, int width, int height);

    void Tick();

    // --- IDeviceNotify---
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // --- ウィンドウメッセージ・イベント ---
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // --- プロパティ ---
    void GetDefaultSize(int& width, int& height) const noexcept;

private:
    // --- 内部処理 ---
    void Update(DX::StepTimer const& timer);
    void Render();
    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

private:
    // --- システムリソース ---
    std::unique_ptr<DX::DeviceResources> m_deviceResources;
    DX::StepTimer m_timer;

    // --- 入力デバイス ---
    DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;
    DirectX::Mouse::ButtonStateTracker m_mouseTracker;

    // --- 描画共通ステート・リソース ---
    std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> m_spriteFont;

    // デバッグ表示用
    std::unique_ptr<DebugFont> m_debugFont;

    // --- シーン管理 ---

    // 各シーンで共有するリソースコンテナ
    std::unique_ptr<UserResources> m_userResources;

    // シーン遷移を管理するマネージャー
    std::unique_ptr<SceneManager<UserResources>> m_sceneManager;

    //フルスクリーン用
    BOOL m_fullscreen;

public:
    //画面モードを設定する関数
    void SetFullscreenState(BOOL value)
    {
        m_fullscreen = value;

        m_deviceResources->GetSwapChain()->SetFullscreenState(m_fullscreen, nullptr);

        if (value)
        {
            m_deviceResources->CreateWindowSizeDependentResources();
        }
    }
};