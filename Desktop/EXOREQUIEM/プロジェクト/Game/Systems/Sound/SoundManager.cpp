/*****************************************************************//**
 * @file    SoundManager.cpp
 * @brief   DirectXTKを用いた音声再生（BGM/SE）の管理の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Sound/SoundManager.h"
#include <chrono>

/**
 * @brief 初期化処理
 */
void SoundManager::Initialize()
{
    DirectX::AUDIO_ENGINE_FLAGS flags = DirectX::AudioEngine_Default;
#ifdef _DEBUG
    flags = flags | DirectX::AudioEngine_Debug;
#endif

    m_audioEngine = std::make_unique<DirectX::AudioEngine>(flags);

	// 初期音量設定
	m_bgmBaseVolumes[L"BGM_Title"]          = VOL_BGM_TITLE;
    m_bgmBaseVolumes[L"BGM_Play"]           = VOL_BGM_PLAY;
    m_bgmBaseVolumes[L"BGM_Standby"]        = VOL_BGM_STANDBY;

    m_seBaseVolumes[L"SE_Decision"]         = VOL_SE_DECISION;
    m_seBaseVolumes[L"SE_Explosion_Hit"]    = VOL_SE_EXPLOSION_HIT;
    m_seBaseVolumes[L"SE_Explosion_Defeat"] = VOL_SE_EXPLOSION_DEF;
    m_seBaseVolumes[L"SE_Missile"]          = VOL_SE_MISSILE;
    m_seBaseVolumes[L"SE_Gun"]              = VOL_SE_GUN;
    m_seBaseVolumes[L"SE_Booster"]          = VOL_SE_BOOSTER;
    m_seBaseVolumes[L"SE_Ready"]            = VOL_SE_READY;
    m_seBaseVolumes[L"SE_Go"]               = VOL_SE_GO;
}

/**
 * @brief 更新処理
 * @details デバイスのロストチェックとエンジンの更新
 */
void SoundManager::Update()
{
    if (!m_audioEngine) return;

    if (!m_audioEngine->Update())
    {
        // オーディオデバイスが失われた場合の処理（ヘッドホンが抜けた等）
        if (m_audioEngine->IsCriticalError())
        {
            // エンジンを一度安全にリセットして、その場で再起動をかける
            m_audioEngine->Reset();
        }
    }
}

/**
 * @brief 終了処理
 */
void SoundManager::Finalize()
{
    if (!m_audioEngine) return;

    m_isFinalizing = true;

    m_audioEngine->Suspend();

    // BGMインスタンスを破棄
    if (m_bgmInstance) {
        m_bgmInstance->Stop(true);
        m_bgmInstance.reset();
    }

    // 最後にエンジンを破棄
    m_audioEngine.reset();
    m_audioEngine = nullptr;

    // 音源データをすべて消去
    m_sounds.clear();
    m_seBaseVolumes.clear();
	m_bgmBaseVolumes.clear();
}

/**
 * @brief 音源ファイルの読み込み
 * @param[in] label 呼び出し用の識別名
 * @param[in] path  wavファイルのパス
 */
void SoundManager::LoadWave(const std::wstring& label, const std::wstring& path)
{
    if (m_isFinalizing || !m_audioEngine) return;

    m_sounds[label] = std::make_unique<DirectX::SoundEffect>(m_audioEngine.get(), path.c_str());
}

/**
 * @brief 効果音(SE)の再生
 * @param[in] label  識別名
 * @param[in] volume 音量(0.0 - 1.0)
 */
void SoundManager::PlaySE(const std::wstring& label, float volume)
{
    if (m_isFinalizing || !m_audioEngine) return;

    if (m_sounds.count(label))
    {
        auto now = std::chrono::steady_clock::now();
        if (m_lastPlayTimes.count(label))
        {
            // 前回の再生時間からの経過時間（ミリ秒）を計算
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPlayTimes[label]).count();

            // 50ミリ秒（0.05秒）以内に同じ音が呼ばれたら、スキップして鳴らさない
            if (elapsed < 50)
            {
                return;
            }
        }

        // 再生時間を「今」に更新
        m_lastPlayTimes[label] = now;

        // SE固有のベース音量を設定（登録がなければ 1.0f）
        float baseVol = m_seBaseVolumes.count(label) ? m_seBaseVolumes[label] : 1.0f;

        // 全てを掛け合わせて最終音量を決定
        float finalVolume = volume * baseVol * m_seVolume;

        m_sounds[label]->Play(finalVolume, 0.0f, 0.0f);
    }
}

/**
 * @brief BGMの再生
 * @details 既存のBGMがある場合は停止してから新しいBGMをループ再生
 * @param[in] label  識別名
 * @param[in] volume 音量(0.0 - 1.0)
 */
void SoundManager::PlayBGM(const std::wstring& label, float volume)
{
    if (m_isFinalizing || !m_audioEngine) return;

    if (m_sounds.count(label))
    {
        // 既に流れているBGMがあれば停止してリセット
        if (m_bgmInstance)
        {
            m_bgmInstance->Stop(true);
            m_bgmInstance.reset();
        }

        // インスタンスを作成してループ再生
        m_bgmInstance = m_sounds[label]->CreateInstance();
        if (m_bgmInstance)
        {
            // BGM固有のベース音量を取得（登録がなければ1.0f）
            float baseVol = m_bgmBaseVolumes.count(label) ? m_bgmBaseVolumes[label] : 1.0f;

            // 全てを掛け合わせて最終音量を決定
            float finalVolume = volume * baseVol * m_bgmVolume;

            m_bgmInstance->SetVolume(finalVolume);
            m_bgmInstance->Play(true); // true = ループ再生AC
        }
    }
}

/**
 * @brief BGMの停止
 */
void SoundManager::StopBGM()
{
    if (m_bgmInstance)
    {
        m_bgmInstance->Stop();
    }
}

/**
 * @brief 中断処理
 */
void SoundManager::OnSuspend()
{
    if (m_audioEngine)
    {
        m_audioEngine->Suspend();
    }
}

/**
 * @brief 再開処理
 */
void SoundManager::OnResume()
{
    if (m_audioEngine)
    {
        m_audioEngine->Resume();
    }
}