/*****************************************************************//**
 * @file    GameplayScene.h
 * @brief   メイン戦闘ステージの進行管理、戦闘開始・終了判定
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Scene/SceneManager.h"
#include "Game/Common/UserResources.h"
#include "Game/Camera/FreeCamera.h"

class World;
class CollisionSystem;
class Skydome;
class InGameUI;
class StageManager;
class LockOnUI;
class PlayerStatusUI;
class EnemyStatusUI;
class PauseManager;
class LockOnSystem;
class ParticleManager;
class DamageIndicator;
class Enemy;
class Player;

class Camera;
class FollowCamera;

// ステージの種類
enum class StageType
{
	Training,
	Stage1,
	Stage2,
	Stage3
};

// メイン戦闘ステージの進行管理、戦闘開始・終了判定
class GameplayScene : public Scene<UserResources>
{
public:
	// 外部（StandbySceneなど）から次のステージを指定するための静的変数
	static StageType s_nextStageType;

	// コンストラクタ
	GameplayScene() noexcept(false);
	// デストラクタ
	~GameplayScene() = default;
	// 初期化処理
	void Initialize() override;
	// 更新処理
	void Update(float elapsedTime) override;
	// 描画処理
	void Render() override;
	// 終了処理
	void Finalize() override;

	// デバイス依存リソース作成
	void CreateDeviceDependentResources() override;
	// ウィンドウサイズ依存リソース作成
	void CreateWindowSizeDependentResources() override;
	// デバイスロスト時処理
	void OnDeviceLost() override;

private:
	// ステージごとの固有データ（敵やマップ）を読み込む関数
	void LoadStageData();
	// 基幹マネージャー・エフェクトの初期化
	void InitializeManagers();
	// ゲームの基幹システム（カメラ、武器、スカイドーム、ワールド）の初期化
	void InitializeGameSystems(const RECT& rect);
	// 衝突判定システムの初期化
	void InitializeCollisionSystem();
	// UI初期化関数
	void InitializeUI();

	// 勝敗条件（クリア・オーバー）の判定
	void CheckGameStatus(float elapsedTime);
	// 終了後の演出とシーン遷移の制御
	void UpdateTransition(float elapsedTime);
	// ゲームクリア時のズーム演出
	void PlayClearTransition(float elapsedTime);
	// シーン遷移の実行
	void ExecuteSceneChange();
	// ポーズ状態の判定と更新
	bool ProcessPause(const DirectX::Keyboard::State& kb, float elapsedTime);
	// ゲーム内主要システムの更新
	void UpdateSystems(const DirectX::Keyboard::State& kb, float elapsedTime);
	// カメラ、エフェクト、UIの更新
	void UpdateEnvironmentAndUI(float elapsedTime);

	// 背景、エンティティ、武器、エフェクトの描画
	void Render3DObjects();
	// ロックオン、ステータス、ポーズ等のUI描画
	void RenderUI();
	// ダメージ演出、デバッグ、最終HUDの描画
	void RenderPostEffects();

private:
	// --- 静的定数パラメータ ---
	// 進行・タイマー関連
	static constexpr float MAX_DELTA_TIME = 0.1f;                         //< 処理落ち対策の最大経過時間制限
	static constexpr float START_DELAY_TIME = 3.0f;                       //< 開幕の待機時間(カウントダウン)
	static constexpr float START_GO_THRESHOLD = 1.5f;                     //< ReadyからGoに切り替わる時間のしきい値
	static constexpr float GAME_OVER_WAIT_TIME = 4.0f;                    //< ゲームオーバー演出からシーン遷移までの待機時間
	static constexpr float GAME_CLEAR_WAIT_TIME = 4.0f;                   //< ゲームクリア演出からシーン遷移までの待機時間

	// カメラ・演出関連
	static constexpr float CAMERA_INIT_YAW = -0.4f;                       //< 初回更新時のカメラYaw初期値
	static constexpr float CAMERA_INIT_PITCH_DEGREES = 15.0f;             //< 初回更新時のカメラPitch初期値(度)
	static constexpr float CAMERA_SHAKE_DURATION = 0.1f;                  //< 被弾時のカメラシェイク時間
	static constexpr float CAMERA_SHAKE_INTENSITY = 0.3f;                 //< 被弾時のカメラシェイク強度
	static constexpr float CLEAR_CAMERA_TARGET_DIST = 4.5f;               //< クリア時のカメラズーム距離
	static constexpr float BOSS_CAMERA_DIST_MULTIPLIER = 5.0f;            //< ボスクリア時のカメラズーム距離倍率
	static constexpr float CLEAR_CAMERA_ZOOM_SPEED = 2.0f;                //< クリア時のズーム速度
	static constexpr float CAMERA_TARGET_OFFSET_Y = -1.0f;                //< クリア時の注視点Yオフセット
	static constexpr float CAMERA_DISTANCE_RESET = -1.0f;                 //< カメラ距離のオーバーライド解除値

	// プロジェクション設定
	static constexpr float CAMERA_FOV_DEGREES = 45.0f;                    //< カメラの視野角（度）
	static constexpr float CAMERA_NEAR_CLIP = 0.5f;                       //< カメラのニアクリップ面
	static constexpr float CAMERA_FAR_CLIP = 10000.0f;                    //< カメラのファークリップ面

	// UI・システム関連
	static constexpr float GUI_BASE_RESOLUTION_HEIGHT = 1080.0f;          //< UIスケール計算用の基準画面高さ
	static constexpr float GUI_DRAW_SCALE_MULTIPLIER = 1.5f;              //< UI描画時の基本スケール倍率
	static constexpr float DEFAULT_PLAYER_MAX_HP = 100.0f;                //< プレイヤー不在時のデフォルト最大HP
	static constexpr float HALF_RATIO = 0.5f;                             //< 中心座標を求めるための乗算係数
	static constexpr unsigned int BLEND_SAMPLE_MASK = 0xFFFFFFFF;         //< 描画時のブレンド用サンプルマスク

	bool m_isUIHidden = false;											  //< UI非表示フラグ（デバッグ用）
	bool m_isFreeCamActive = false;										  //< フリーカメラ有効フラグ（デバッグ用）

	// --- 行列・カメラ ---
	DirectX::SimpleMath::Matrix m_proj;                                   //< 射影行列
	DirectX::SimpleMath::Matrix m_view;                                   //< ビュー行列
	std::unique_ptr<Camera> m_camera;									  //< プレイヤーカメラ
	std::unique_ptr<FreeCamera> m_freeCamera;							  //< フリーカメラ
	FollowCamera* m_followCamera;                                         //< カメラ追従ポインタ

	std::unique_ptr<DirectX::BasicEffect> m_basicEffect;                  //< 基本エフェクト
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;              //< 入力レイアウト

	// --- システム管理・ワールド ---
	StageType m_currentStageType;                                         //< このシーンインスタンスが現在担当しているステージ
	std::unique_ptr<World> m_world;                                       //< ワールド
	std::unique_ptr<StageManager> m_stageManager;                         //< ステージ管理
	std::unique_ptr<CollisionSystem> m_collisionManager;                  //< 衝突判定
	std::unique_ptr<LockOnSystem> m_lockOnSystem;                         //< ロックオンシステム
	std::unique_ptr<ParticleManager> m_explosionEffect;                   //< 爆発エフェクト
	std::unique_ptr<Skydome> m_skydome;                                   //< 天球

	// --- UI管理 ---
	std::unique_ptr<InGameUI> m_inGameUI;                                 //< インゲームUI
	std::unique_ptr<DamageIndicator> m_damageIndicator;                   //< ダメージインジケーター
	std::unique_ptr<LockOnUI> m_lockOnUI;                                 //< ロックオンUI
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;                  //< スプライトバッチ
	std::unique_ptr<PlayerStatusUI> m_playerStatusUI;                     //< プレイヤーステータスUI
	std::unique_ptr<EnemyStatusUI> m_enemyStatusUI;                       //< 敵ステータスUI
	std::unique_ptr<PauseManager> m_pauseManager;                         //< ポーズマネージャー

	// --- ゲーム進行状態・タイマー ---
	DirectX::SimpleMath::Vector3 m_finalTargetPosition;                   //< 最後に倒した/生き残っている敵へのポインタ
	float m_startDelayTimer;                                              //< 開始ディレイ
	float m_transitionTimer;                                              //< 遷移タイマー
	float m_gameTimer;                                                    //< ゲーム経過時間
	bool m_hasFinalTargetPosition = false;								  //< 座標が有効かどうかのフラグ

	bool m_isGameClear;                                                   //< クリアフラグ
	bool m_isGameOver;                                                    //< オーバーフラグ
	bool m_isFirstUpdate;                                                 //< 初回更新フラグ
	bool m_hasPlayedReadyGoSE;                                            //< ReadyGoSE再生フラグ

	// --- リソース ---
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texReady;          //< Readyテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texGo;             //< Goテクスチャ
	DirectX::SimpleMath::Vector2 m_sizeReady;                             //< Readyテクスチャのサイズ
	DirectX::SimpleMath::Vector2 m_sizeGo;                                //< Goテクスチャのサイズ
};