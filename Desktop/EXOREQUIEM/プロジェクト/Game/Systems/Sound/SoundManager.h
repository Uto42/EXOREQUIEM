/*****************************************************************//**
 * @file    SoundManager.h
 * @brief   DirectXTKを用いた音声再生（BGM/SE）の管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <Audio.h>
#include <map>
#include <string>

class SoundManager
{
public:

    // コンストラクタ
    SoundManager() = default;
    // デストラクタ
    ~SoundManager()
    {
    };

    // どこからでも呼べるようにシングルトン化
    static SoundManager& Instance()
    {
        static SoundManager instance;
        return instance;
    }

    // 初期化処理
    void Initialize();
    // 更新処理
    void Update();
    // 終了処理
    void Finalize();

    // 音源の読み込み
    void LoadWave(const std::wstring& label, const std::wstring& path);
    // SEの再生
    void PlaySE(const std::wstring& label, float volume = 1.0f);
    // BGMの再生
    void PlayBGM(const std::wstring& label, float volume = 0.5f);
    // BGMの停止
    void StopBGM();
    // サスペンド
    void OnSuspend();
    // レジューム
    void OnResume();

private:
    // --- 定数 ---
    static constexpr float DEFAULT_BASE_VOLUME = 1.0f;      //< 音量マップに未登録の際のデフォルト倍率
    static constexpr float PITCH_PAN_DEFAULT = 0.0f;        //< ピッチとパンのデフォルト値

    // --- システム全体の初期マスターボリューム ---
    static constexpr float INITIAL_MASTER_BGM_VOL = 0.3f;   //< BGM全体の初期音量
    static constexpr float INITIAL_MASTER_SE_VOL = 0.1f;    //< SE全体の初期音量

    // --- 各BGMの固有ベース音量 ---
	static constexpr float VOL_BGM_TITLE = 0.5f;            //< タイトルBGMのベース音量
	static constexpr float VOL_BGM_PLAY = 0.5f; 		    //< プレイ中BGMのベース音量 
	static constexpr float VOL_BGM_STANDBY = 0.5f;	        //< スタンバイ中BGMのベース音量

    // --- 各SEの固有ベース音量 ---
	static constexpr float VOL_SE_DECISION = 1.0f;	        //< 決定SEのベース音量
	static constexpr float VOL_SE_EXPLOSION_HIT = 0.5f;     //< 爆発ヒットSEのベース音量
	static constexpr float VOL_SE_EXPLOSION_DEF = 0.5f;	    //< 爆発防御SEのベース音量
	static constexpr float VOL_SE_MISSILE = 0.4f;           //< ミサイル発射SEのベース音量
	static constexpr float VOL_SE_GUN = 0.4f;               //< ガン発射SEのベース音量
	static constexpr float VOL_SE_BOOSTER = 1.0f;           //< ブースターSEのベース音量
    static constexpr float VOL_SE_READY = 2.0f;             //< システムボイス等は大きめに設定
	static constexpr float VOL_SE_GO = 1.0f;                //< ゲーム開始SEのベース音量

    // --- メンバ変数 ---
    std::unique_ptr<DirectX::AudioEngine>                         m_audioEngine;    //< オーディオエンジン
    std::map<std::wstring, std::unique_ptr<DirectX::SoundEffect>> m_sounds;         //< 読み込み済みSEリスト
    std::unique_ptr<DirectX::SoundEffectInstance>                 m_bgmInstance;    //< 現在再生中のBGMインスタンス
    std::map<std::wstring, std::chrono::steady_clock::time_point> m_lastPlayTimes;  //< 最後に再生した時間」を記録

	std::map<std::wstring, float> m_seBaseVolumes;                                  //< SEごとの基本音量
	std::map<std::wstring, float> m_bgmBaseVolumes;                                 //< BGMごとの基本音量

    bool m_isFinalizing = false;                                                    //< 終了処理中フラグ

    float m_bgmVolume = INITIAL_MASTER_BGM_VOL;                                     //< マスターBGM音量
    float m_seVolume = INITIAL_MASTER_SE_VOL;                                       //< マスターSE音量
};