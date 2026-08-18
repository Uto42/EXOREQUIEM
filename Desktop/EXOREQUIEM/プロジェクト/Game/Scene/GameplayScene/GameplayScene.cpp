/*****************************************************************//**
 * @file    GameplayScene.cpp
 * @brief   メイン戦闘ステージの進行管理、戦闘開始・終了判定
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include <random>

#include "GameplayScene.h"
#include "Game/Scene/TitleScene/TitleScene.h"
#include "Game/Scene/StandbyScene/StandbyScene.h"
#include "Game/Scene/LoadingScreen/GameLoadingScreen.h"
#include "Game/Scene/ResultScene/ResultScene.h"

#include "Game/Systems/Sound/SoundManager.h"
#include "Game/Systems/Collision/CollisionSystem.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Systems/Effect/Particle/ParticleManager.h"
#include "Game/Systems/Effect/Visuals/DamageIndicator.h"
#include "Game/Systems/Pause/PauseManager.h"

#include "Game/Stage/Skydome.h"
#include "Game/Stage/StageManager.h"

#include "Game/UI/HUD/InGameUI.h"
#include "Game/UI/HUD/LockOnUI.h"
#include "Game/UI/Status/PlayerStatusUI.h"
#include "Game/UI/Status/EnemyStatusUI.h"

#include "Game/World/World.h"
#include "Game/Systems/LockOn/LockOnSystem.h"
#include "Game/GameObjects/Enemy/Enemy.h"
#include "Game/GameObjects/Player/Player.h"

#include "Game/Camera/Camera.h"
#include "Game/Camera/FollowCamera.h" 
#include "Game/Camera/LockOnCamera.h"

// 実体化（デフォルトはStage1にしておく）
StageType GameplayScene::s_nextStageType = StageType::Stage1;

/**
 * @brief コンストラクタ
 */
GameplayScene::GameplayScene() noexcept(false)
	: m_view(DirectX::SimpleMath::Matrix::Identity)
	, m_followCamera(nullptr)
	, m_isGameClear(false)
	, m_isGameOver(false)
	, m_transitionTimer(0.0f)
	, m_gameTimer(0.0f)
	, m_isFirstUpdate(true)
	, m_hasFinalTargetPosition(nullptr)
	, m_hasPlayedReadyGoSE(false)
	, m_startDelayTimer(START_DELAY_TIME)
{
	// 生成された瞬間に、指定されていたステージタイプを受け取る
	m_currentStageType = s_nextStageType;
}

/**
 * @brief 初期化処理
 */
void GameplayScene::Initialize()
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();

	// Direct3Dコンテキストを取得
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();
	RECT rect = GetUserResources()->GetDeviceResources()->GetOutputSize();

	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(context);

	// 各マネージャー（ステータスUI、エフェクト、ステージ）を初期化
	InitializeManagers();
	// ゲームの基幹システム（カメラ、武器、スカイドーム、ワールド）を初期化
	InitializeGameSystems(rect);
	// 衝突判定とダメージインジケーターを初期化
	InitializeCollisionSystem();
	// ロックオンUI、ポーズ、HUDなどのUI一式を初期化
	InitializeUI();

	// ステージの種類に応じて敵やマップの読み込みを分ける
	LoadStageData();
}

/**
 * @brief ステージの種類に応じた敵データのロード
 */
void GameplayScene::LoadStageData()
{
	switch (m_currentStageType)
	{
	case StageType::Training:
		m_world->CreateTrainingEnemies();
		break;
	case StageType::Stage1:
		m_world->CreateStage1Enemies();
		break;
	case StageType::Stage2:
		m_world->CreateStage2Enemies();
		break;
	case StageType::Stage3:
		m_world->CreateStage3Enemies();
		break;
	}
}

/**
 * @brief 基幹マネージャー・エフェクトの初期化
 */
void GameplayScene::InitializeManagers()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	// UIマネージャーの生成と初期化
	m_playerStatusUI = std::make_unique<PlayerStatusUI>();
	m_playerStatusUI->Initialize(device);
	// 敵ステータスUIの生成と初期化
	m_enemyStatusUI = std::make_unique<EnemyStatusUI>();
	m_enemyStatusUI->Initialize(device);

	// ステージマネージャーの生成と初期化
	m_stageManager = std::make_unique<StageManager>();
	m_stageManager->Initialize(device, context, static_cast<int>(m_currentStageType));

	// 爆発エフェクト用のパーティクルマネージャーを生成
	m_explosionEffect = std::make_unique<ParticleManager>();
	m_explosionEffect->Create(device, context);

	EffectSystem::Instance()->Initialize(device, context);
}

/**
 * @brief ゲームシステム（カメラ・ワールド・武器）の生成
 * @param[in] rect ウィンドウの表示領域（カメラの解像度設定に使用）
 */
void GameplayScene::InitializeGameSystems(const RECT& rect)
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	// フリーカメラの生成（デバッグ用）
	m_freeCamera = std::make_unique<FreeCamera>();
	// カメラシステムの生成
	m_camera = std::make_unique<Camera>(rect.right, rect.bottom);

	// 背景（天球）オブジェクトの生成
	m_skydome = std::make_unique<Skydome>();

	// ロックオン制御システムの生成とステージ情報の紐付け
	m_lockOnSystem = std::make_unique<LockOnSystem>();
	m_lockOnSystem->SetStageManager(m_stageManager.get());

	// ワールド（エンティティ管理者）の生成と初期化
	m_world = std::make_unique<World>();
	m_world->Initialize(device, context, m_lockOnSystem.get(), m_stageManager.get());

	// ステージに応じて背景モデルを切り替える
	if (m_skydome)
	{
		const wchar_t* skydomeFile = L"Skydome_01.sdkmesh";

		switch (m_currentStageType)
		{
		case StageType::Stage1:
			skydomeFile = L"Skydome_01.sdkmesh";
			break;
		case StageType::Stage2:
			skydomeFile = L"Skydome_02.sdkmesh";
			break;
		}

		m_skydome->Initialize(device, m_world.get(), skydomeFile);
	}

	// プレイヤーが生成されていれば、ロックオンの基準点として初期設定する
	if (Player* player = m_world->GetPlayer()) 
	{
		m_lockOnSystem->Initialize(player->GetRobot());
	}

	// カメラコンポーネント（追従・ロックオン機能）の追加
	if (m_camera)
	{
		auto follower = std::make_unique<FollowCamera>(m_world.get());

		// クリア時のズーム演出等で直接操作するためにポインタをキャッシュしておく
		m_followCamera = follower.get();
		m_camera->AddComponent(std::move(follower));
		
		// ロックオンカメラコンポーネントを追加することで、ロックオン対象の敵を追従するようになる
		auto lockOnComp = std::make_unique<LockOnCamera>(m_world.get(), m_lockOnSystem.get());
		m_camera->AddComponent(std::move(lockOnComp));
	}
}

/**
 * @brief 衝突判定マネージャーの初期化
 */
void GameplayScene::InitializeCollisionSystem()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();

	// 衝突判定システムの生成と初期化
	m_collisionManager = std::make_unique<CollisionSystem>(m_world.get());

	// ダメージインジケーターの生成と初期化
	m_damageIndicator = std::make_unique<DamageIndicator>();
	m_damageIndicator->Initialize(device);

	// プレイヤーが攻撃を受けた際の演出（カメラの揺れやUI表示）を登録する
	if (Player* player = m_world->GetPlayer())
	{
		if (auto robot = player->GetRobot())
		{
			robot->SetOnHitCallback([this, player](const DirectX::SimpleMath::Vector3& shooterPos)
			{
				// カメラを揺らしてダメージを受けた衝撃を表現する
				if (m_camera)
				{
					m_camera->StartShake(CAMERA_SHAKE_INTENSITY, CAMERA_SHAKE_DURATION);
				}

				// 攻撃が飛んできた方向を計算し、画面上に赤いマークを表示する
				if (m_damageIndicator && m_camera)
				{
					// カメラの向き（ビュー行列の逆行列）を取得し、自機から見た相対的な被弾方向を計算する
					DirectX::SimpleMath::Matrix invView = m_camera->GetViewMatrix().Invert();
					DirectX::SimpleMath::Vector3 camRight = invView.Right();
					DirectX::SimpleMath::Vector3 camForward = invView.Forward();

					// 被弾方向を計算してダメージインジケーターに通知する
					m_damageIndicator->AddHit(player->GetPosition(), camForward, camRight, shooterPos);
				}
			});
		}
	}
}

/**
 * @brief UI関連の初期化
 */
void GameplayScene::InitializeUI()
{
	auto device = GetUserResources()->GetDeviceResources()->GetD3DDevice();
	auto context = GetUserResources()->GetDeviceResources()->GetD3DDeviceContext();

	// ロックオンUIの生成と初期化
	m_lockOnUI = std::make_unique<LockOnUI>();
	m_lockOnUI->Initialize(device, context);
	// インゲームUIの生成と初期化
	m_inGameUI = std::make_unique<InGameUI>();
	m_inGameUI->Initialize(device, context, m_world.get(), m_camera.get(), m_lockOnSystem.get(), m_lockOnUI.get());
	// トレーニングモード時のUIの生成
	m_inGameUI->SetTrainingMode(m_currentStageType == StageType::Training);

	// ポーズマネージャーの生成と初期化
	m_pauseManager = std::make_unique<PauseManager>();
	m_pauseManager->Initialize(device);
}

/**
 * @brief 更新処理
 * @param[in] elapsedTime 前フレームからの経過時間（秒）
 */
void GameplayScene::Update(float elapsedTime)
{
	// マウスカーソルを非表示に設定
	while (ShowCursor(FALSE) >= 0);

	auto kbTracker = GetUserResources()->GetKeyboardStateTracker();
	const auto kb = kbTracker->GetLastState();

	// 処理落ちなどで経過時間が異常に長くなった場合、ゲームの挙動が破綻しないように上限を設ける
	elapsedTime = std::min(elapsedTime, MAX_DELTA_TIME);

	// 初回更新時のみ、開始前の準備処理を実行
	if (m_isFirstUpdate)
	{
		SoundManager::Instance().StopBGM();
		SoundManager::Instance().PlaySE(L"SE_Ready");

		// カメラを指定した初期向きに強制リセットする
		if (m_camera)
		{
			constexpr float initPitch = DirectX::XMConvertToRadians(CAMERA_INIT_PITCH_DEGREES);
			m_camera->SetCameraYawPitch(CAMERA_INIT_YAW, initPitch);
		}

		// マウスの物理的なカーソル位置をウィンドウの中心に強制移動させる
		HWND hWnd = GetUserResources()->GetDeviceResources()->GetWindow();
		RECT rect;
		GetClientRect(hWnd, &rect);

		// ウィンドウのクライアント領域の中心座標を計算し、スクリーン座標に変換してカーソル位置を設定
		POINT center = { (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };
		ClientToScreen(hWnd, &center);
		SetCursorPos(center.x, center.y);

		m_isFirstUpdate = false;
	}

	// 開幕の「READY」表示タイマーをカウントダウン
	if (m_startDelayTimer > 0.0f)
	{
		m_startDelayTimer -= elapsedTime;
	}

	// タイマーが閾値を切ったら「GO」の音とメインBGMを再生する
	if (m_startDelayTimer <= START_GO_THRESHOLD && !m_hasPlayedReadyGoSE)
	{
		SoundManager::Instance().PlaySE(L"SE_Go");
		SoundManager::Instance().PlayBGM(L"BGM_Play");

		m_hasPlayedReadyGoSE = true;
	}

	// デバッグや非常時用のタイトルへ戻る強制処理
	if (kbTracker->IsKeyPressed(DirectX::Keyboard::Back))
	{
		SoundManager::Instance().StopBGM();
		ChangeScene<StandbyScene, GameLoadingScreen>();
		return;
	}

	// ゲームの勝敗が決まったか判定
	CheckGameStatus(elapsedTime);

	// 終了フラグが立っている場合の演出更新とシーン遷移
	UpdateTransition(elapsedTime);

	// ポーズ中であればこれ以降の更新処理をスキップ
	if (ProcessPause(kb, elapsedTime)) return;

	// タイマーが一定時間を切る（GOの合図）まではキャラなどの更新をストップさせる
	if (m_startDelayTimer <= START_GO_THRESHOLD)
	{
		// プレイヤー、敵、武器、衝突判定などの動的要素を更新
		UpdateSystems(kb, elapsedTime);
	}

	// カメラ、スカイドーム、UIの表示を更新
	UpdateEnvironmentAndUI(elapsedTime);
}

/**
 * @brief 勝敗条件（クリア・ゲームオーバー）の判定
 * @param[in] elapsedTime 前フレームからの経過時間（秒）
 */
void GameplayScene::CheckGameStatus(float elapsedTime)
{
	// すでに決着がついている場合は何もしない
	if (m_isGameOver || m_isGameClear) return;

	m_gameTimer += elapsedTime;
	Player* player = m_world->GetPlayer();

	// プレイヤーのHPがゼロになったら敗北
	if (player && player->GetHealth() <= 0.0f)
	{
		SoundManager::Instance().StopBGM();
		m_isGameOver = true;
		m_transitionTimer = 0.0f;
		ResultScene::s_isGameClear = false;
		return;
	}

	// ステージ上の生きている敵を探して勝利条件を満たしたか判定する
	bool isAnyEnemyAlive = false;

	if (auto enemyManager = m_world->GetEnemyManager())
	{
		const auto& enemies = enemyManager->GetEnemies();
		for (const auto& enemy : enemies)
		{
			if (enemy->IsActive() && enemy->GetHealth() > 0.0f)
			{
				isAnyEnemyAlive = true;
				if (auto robot = enemy->GetRobot())
				{
					m_finalTargetPosition = robot->GetAimPosition();
					m_hasFinalTargetPosition = true;
				}
				break;
			}
		}
	}

	// 生きている敵がいなければクリア確定
	if (!isAnyEnemyAlive && m_hasFinalTargetPosition)
	{
		// トレーニングモードの場合は、すべての敵のHPを回復させて復活させる
		if (m_currentStageType == StageType::Training)
		{
			if (auto enemyMgr = m_world->GetEnemyManager())
			{
				enemyMgr->ResetAllEnemies();
			}
		}
		// 通常ステージの場合はクリア処理へ移行
		else
		{
			m_isGameClear = true;
			m_transitionTimer = 0.0f;
			SoundManager::Instance().StopBGM();

			// リザルトシーンへ渡すスコア等のデータをセット
			ResultScene::s_isGameClear = true;
			ResultScene::s_clearTime = m_gameTimer;
			ResultScene::s_remainingHP = player ? player->GetHealth() : 0.0f;
			ResultScene::s_maxHP = player ? player->GetMaxHealth() : DEFAULT_PLAYER_MAX_HP;
		}
	}
}

/**
 * @brief ゲーム終了後の演出とシーン遷移の制御
 * @param[in] elapsedTime 前フレームからの経過時間（秒）
 */
void GameplayScene::UpdateTransition(float elapsedTime)
{
	if (!m_isGameClear && !m_isGameOver) return;

	m_transitionTimer += elapsedTime;

	if (m_isGameClear)
	{
		PlayClearTransition(elapsedTime);
	}

	// 規定の待機時間が経過したらリザルト画面へ遷移する
	float targetWaitTime = m_isGameOver ? GAME_OVER_WAIT_TIME : GAME_CLEAR_WAIT_TIME;
	if (m_transitionTimer >= targetWaitTime)
	{
		ExecuteSceneChange();
	}
}

/**
 * @brief ゲームクリア時の演出（敵へのズームイン）
 * @param[in] elapsedTime 前フレームからの経過時間（秒）
 */
void GameplayScene::PlayClearTransition(float elapsedTime)
{
	if (m_followCamera && m_hasFinalTargetPosition)
	{
		m_followCamera->SetTargetOverride(true, m_finalTargetPosition);

		float currentDist = m_followCamera->GetCurrentCameraDistance();
		float targetDist = CLEAR_CAMERA_TARGET_DIST;

		// ボスはモデルが巨大なため、通常の敵よりもカメラを遠ざけて画面に収める
		if (m_currentStageType == StageType::Stage3)
		{
			targetDist = CLEAR_CAMERA_TARGET_DIST * BOSS_CAMERA_DIST_MULTIPLIER;
		}

		// 現在の距離から目標の距離へ滑らかにズームさせる
		float newDist = currentDist + (targetDist - currentDist) * elapsedTime * CLEAR_CAMERA_ZOOM_SPEED;

		m_followCamera->SetDistanceOverride(newDist);
	}
}

/**
 * @brief カメラ設定のリセットとシーン遷移の実行
 */
void GameplayScene::ExecuteSceneChange()
{
	// 次回プレイ時に影響が出ないようカメラの強制設定を解除しておく
	if (m_followCamera)
	{
		m_followCamera->SetTargetOverride(false);
		m_followCamera->SetDistanceOverride(CAMERA_DISTANCE_RESET);
	}

	ResultScene::s_isGameClear = m_isGameClear;
	ChangeScene<ResultScene, GameLoadingScreen>();
}

/**
 * @brief ポーズ状態の判定と更新
 * @param[in] kb キーボードの状態
 * @param[in] elapsedTime 前フレームからの経過時間（秒）
 * @return ポーズ中の場合は true、進行中の場合は false
 */
bool GameplayScene::ProcessPause(const DirectX::Keyboard::State& kb, float elapsedTime)
{
	HWND hWnd = GetUserResources()->GetDeviceResources()->GetWindow();

	// ウィンドウがフォーカスを失った（非アクティブになった）場合は自動でポーズをかける
	if (GetForegroundWindow() != hWnd)
	{
		m_pauseManager->SetPaused(true);
	}

	bool isPaused = m_pauseManager->Update(kb, elapsedTime);

	// ポーズメニューで選択された項目に応じた処理を実行する
	auto executedMenu = m_pauseManager->GetExecutedMenu();
	if (executedMenu != PauseManager::MenuType::None)
	{
		switch (executedMenu)
		{
		case PauseManager::MenuType::Retry:
			SoundManager::Instance().StopBGM();
			ChangeScene<GameplayScene, GameLoadingScreen>();
			break;

		case PauseManager::MenuType::Title:
			SoundManager::Instance().StopBGM();
			ChangeScene<StandbyScene, GameLoadingScreen>();
			break;

		case PauseManager::MenuType::Quit:
			PostQuitMessage(0);
			break;

		default:
			break;
		}

		m_pauseManager->ClearExecutedMenu();
	}

	return isPaused;
}

/**
 * @brief ゲーム内主要システムの更新
 * @param[in] kb キーボードの状態
 * @param[in] elapsedTime 前フレームからの経過時間（秒）
 */
void GameplayScene::UpdateSystems(const DirectX::Keyboard::State& kb, float elapsedTime)
{
	auto kbTracker = GetUserResources()->GetKeyboardStateTracker();
	auto playerRobot = m_world ? m_world->GetPlayer() : nullptr;

	// F3キーによるオートパイロット（AIモード）への切り替え機能
	if (playerRobot && kbTracker->IsKeyPressed(DirectX::Keyboard::F3))
	{
		bool nextAutoMode = !playerRobot->IsAutoPilot();

		const Robot* target = nullptr;
		if (nextAutoMode)
		{
			// AIモードにする場合、一番最初に見つかった生きている敵をターゲットに設定する
			if (auto enemyMgr = m_world->GetEnemyManager())
			{
				for (const auto& enemy : enemyMgr->GetEnemies())
				{
					if (enemy && enemy->IsActive() && enemy->GetHealth() > 0.0f)
					{
						target = enemy->GetRobot();
						break;
					}
				}
			}
		}

		playerRobot->SetAutoPilot(nextAutoMode, m_stageManager.get(), target);
	}

	// ワールドの更新
	if (m_world) m_world->Update(elapsedTime, kb);

	// ロックオンシステムの更新
	if (m_world && m_lockOnSystem)
	{
		auto enemyMgr = m_world->GetEnemyManager();
		auto player = m_world->GetPlayer();

		if (enemyMgr)
		{
			const auto& enemies = enemyMgr->GetEnemies();
			std::vector<Robot*> targetRobots;
			targetRobots.reserve(enemies.size());

			for (const auto& enemy : enemies)
			{
				if (enemy)
				{
					targetRobots.push_back(enemy->GetRobot());
				}
			}

			float mouseDeltax = 0.0f;
			float mouseDeltay = 0.0f;

			if (m_camera)
			{
				mouseDeltax = m_camera->GetMouseDeltaX();
				mouseDeltay = m_camera->GetMouseDeltaY();
			}

			m_lockOnSystem->Update(targetRobots, player->GetCameraForward(), mouseDeltax, mouseDeltay, elapsedTime);

			// Tabキーによるハードロック（ターゲット固定）の切り替え
			if (kbTracker->IsKeyPressed(DirectX::Keyboard::Tab))
			{
				m_lockOnSystem->ToggleHardLock();
			}
		}
	}

	// プレイヤーが攻撃対象になり得る状態であれば衝突判定を実行する
	if (m_collisionManager && !m_isGameClear && !m_isGameOver)
	{
		auto player = m_world ? m_world->GetPlayer() : nullptr;
		if (player && player->IsTargetable())
		{
			m_collisionManager->Update();
		}
	}

	// ダメージインジケーターの更新
	if (m_damageIndicator)
	{
		m_damageIndicator->Update(elapsedTime);
	}
}

/**
 * @brief カメラ、エフェクト、UIの更新
 * @param[in] elapsedTime 前フレームからの経過時間（秒）
 */
void GameplayScene::UpdateEnvironmentAndUI(float elapsedTime)
{
	auto kbTracker = GetUserResources()->GetKeyboardStateTracker();

	// F1キー：UI非表示の切り替え
	if (kbTracker->IsKeyPressed(DirectX::Keyboard::F1))
	{
		m_isUIHidden = !m_isUIHidden;
	}

	// F2キー：フリーカメラの切り替え
	if (kbTracker->IsKeyPressed(DirectX::Keyboard::F2))
	{
		m_isFreeCamActive = !m_isFreeCamActive;

		// フリーカメラ中は自機の透明化を無効（強制不透明）にする
		if (m_world && m_world->GetPlayer())
		{
			m_world->GetPlayer()->SetForceOpaque(m_isFreeCamActive);
		}

		// フリーカメラ起動時、メインカメラの現在地を引き継ぐ
		if (m_isFreeCamActive && m_camera)
		{
			m_freeCamera->Initialize(
				m_camera->GetPosition(),
				m_camera->GetYaw(),
				m_camera->GetPitch()
			);
		}
	}

	// カメラの更新と行列の設定
	if (m_isFreeCamActive && m_freeCamera)
	{
		// フリーカメラモード
		m_freeCamera->Update(elapsedTime);
		m_view = m_freeCamera->GetViewMatrix();
	}
	else if (m_camera)
	{
		// 通常カメラモード
		m_camera->Update(elapsedTime);
		m_view = m_camera->GetViewMatrix();
	}

	// ロックオンUIの更新
	if (m_lockOnUI)
	{
		m_lockOnUI->Update(elapsedTime);
	}

	// エフェクトの更新
	EffectSystem::Instance()->Update(elapsedTime);

	if (m_skydome)
	{
		m_skydome->Update();
	}

	// UIの更新
	if (m_inGameUI)
	{
		auto deviceResources = GetUserResources()->GetDeviceResources();
		DirectX::SimpleMath::Viewport viewport(deviceResources->GetScreenViewport());
		m_inGameUI->Update(elapsedTime, deviceResources->GetOutputSize(), viewport);
	}
}

/**
 * @brief 描画処理
 */
void GameplayScene::Render()
{
	// 描画前の共通設定
	Render3DObjects();
	// UI描画
	RenderUI();
	// ポストエフェクト、インゲームHUDの描画
	RenderPostEffects();
}

/**
 * @brief 3Dオブジェクト（背景、キャラ、武器、エフェクト）の描画
 */
void GameplayScene::Render3DObjects()
{
	auto userResources = GetUserResources();
	auto context = userResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = userResources->GetCommonStates();

	// 最背面に描画されるスカイドーム
	if (m_skydome)
	{
		m_skydome->Render(context, states, m_view, m_proj);
	}

	// 地形、建物などのステージオブジェクト
	if (m_stageManager)
	{
		m_stageManager->Render(context, states, m_view, m_proj);
	}

	// キャラクター本体のモデル描画
	if (m_world)
	{
		m_world->Render(context, states, m_view, m_proj);
	}

	// パーティクルなどのエフェクト描画
	EffectSystem::Instance()->Render(context, m_view, m_proj);
}

/**
 * @brief 2D UI（ロックオン、ステータス、ポーズメニュー）の描画
 */
void GameplayScene::RenderUI()
{
	auto userResources = GetUserResources();
	auto states = userResources->GetCommonStates();
	auto deviceResources = userResources->GetDeviceResources();
	D3D11_VIEWPORT d3dViewport = deviceResources->GetScreenViewport();

	float guiScale = d3dViewport.Height / GUI_BASE_RESOLUTION_HEIGHT;

	m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, states->NonPremultiplied());

	// UI非表示モードでなければ、各種UIを描画する
	if (!m_isUIHidden)
	{
		// READY表示中はすべてのゲームUIを非表示にし、GO以降でまとめて描画する
		if (m_startDelayTimer <= START_GO_THRESHOLD)
		{
			// ロックオン・クロスヘアUI
			if (m_lockOnUI && m_lockOnSystem && m_camera)
			{
				m_lockOnUI->Render(m_spriteBatch.get(), m_lockOnSystem.get(), m_view, m_proj, d3dViewport);
			}

			// プレイヤーのHP・エネルギーゲージ等のステータスUI
			if (m_playerStatusUI && m_world)
			{
				m_playerStatusUI->Render(m_spriteBatch.get(), m_world->GetPlayer(), d3dViewport);
			}

			// 敵のステータスUI
			if (m_enemyStatusUI && m_world)
			{
				if (auto enemyMgr = m_world->GetEnemyManager())
				{
					m_enemyStatusUI->Render(m_spriteBatch.get(), enemyMgr->GetEnemies(), m_view, m_proj, d3dViewport);
				}
			}
		}

		// 「READY / GO」画像の描画
		if (m_startDelayTimer > 0.0f)
		{
			auto& currentTex = (m_startDelayTimer > START_GO_THRESHOLD) ? m_texReady : m_texGo;
			auto& currentSize = (m_startDelayTimer > START_GO_THRESHOLD) ? m_sizeReady : m_sizeGo;

			if (currentTex)
			{
				// 画面の中心座標
				DirectX::SimpleMath::Vector2 screenCenter(
					d3dViewport.Width * HALF_RATIO, d3dViewport.Height * HALF_RATIO);

				// 画像の中心を基準点（ピボット）とする
				DirectX::SimpleMath::Vector2 origin = currentSize * HALF_RATIO;
				float drawScale = GUI_DRAW_SCALE_MULTIPLIER * guiScale;

				// 「READY / GO」画像を画面中央に描画する
				m_spriteBatch->Draw(currentTex.Get(), screenCenter, nullptr,
					DirectX::Colors::White, 0.0f, origin, drawScale);
			}
		}
	}

	m_spriteBatch->End();
}

/**
 * @brief ポストエフェクト、デバッグ情報、インゲームHUDの描画
 */
void GameplayScene::RenderPostEffects()
{
	auto userResources = GetUserResources();
	auto context = userResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = userResources->GetCommonStates();

	// UI非表示モードにしている場合は、HUDや被弾演出などの描画をスキップする
	if (!m_isUIHidden)
	{
		// 被弾時の画面赤転などの演出
		if (m_damageIndicator) m_damageIndicator->Render(context);

		// READY表示中はリロードバー等のHUD（InGameUI）を非表示にし、GO以降で描画する
		if (m_startDelayTimer <= START_GO_THRESHOLD)
		{
			if (m_inGameUI) m_inGameUI->Render();
		}
	}

	// デバッグ用描画（当たり判定など）
	if (m_stageManager) m_stageManager->RenderDebug(context, states, m_view, m_proj);

	// ポーズ画面は他のすべてのUIの上に描画するため、最後に独立して実行する
	if (m_pauseManager)
	{
		m_spriteBatch->Begin(DirectX::SpriteSortMode_Deferred, states->NonPremultiplied());

		RECT outputSize = userResources->GetDeviceResources()->GetOutputSize();
		m_pauseManager->Render(m_spriteBatch.get(), outputSize);

		m_spriteBatch->End();
	}
}

/**
 * @brief 終了処理
 */
void GameplayScene::Finalize()
{
}

/**
 * @brief デバイス依存リソースの作成
 */
void GameplayScene::CreateDeviceDependentResources()
{
	auto userResources = GetUserResources();
	auto device = userResources->GetDeviceResources()->GetD3DDevice();

	// DirectXTKのSpriteBatchを使用して2Dスプライト描画を行うための初期化
	m_basicEffect = std::make_unique<DirectX::BasicEffect>(device);
	m_basicEffect->SetTextureEnabled(true);

	// DirectXTKのBasicEffectを使用する場合、入力レイアウトを作成する必要がある
	DX::ThrowIfFailed(
		DirectX::CreateInputLayoutFromEffect<DirectX::VertexPositionNormalTexture>(
			device, m_basicEffect.get(), m_inputLayout.ReleaseAndGetAddressOf())
	);

	// UI用のテクスチャをロードするためのラムダ関数
	auto LoadUIImg = [&](const wchar_t* path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& tex,
		DirectX::SimpleMath::Vector2& sizeOut)
	{
		DX::ThrowIfFailed(DirectX::CreateWICTextureFromFile(device, path, nullptr, tex.ReleaseAndGetAddressOf()));

		Microsoft::WRL::ComPtr<ID3D11Resource> res;
		tex->GetResource(res.GetAddressOf());
		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
		if (SUCCEEDED(res.As(&tex2D))) 
		{
			D3D11_TEXTURE2D_DESC desc;
			tex2D->GetDesc(&desc);
			sizeOut = DirectX::SimpleMath::Vector2(static_cast<float>(desc.Width), static_cast<float>(desc.Height));
		}
	};

	// UI用のテクスチャをロード
	LoadUIImg(L"Resources/Textures/inGame/inGame_font_ready.png", m_texReady, m_sizeReady);
	LoadUIImg(L"Resources/Textures/inGame/inGame_font_go.png", m_texGo, m_sizeGo);
}

/**
 * @brief ウィンドウサイズ依存リソースの作成
 */
void GameplayScene::CreateWindowSizeDependentResources()
{
	RECT rect = GetUserResources()->GetDeviceResources()->GetOutputSize();
	float aspectRatio = static_cast<float>(rect.right) / static_cast<float>(rect.bottom);

	m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		DirectX::XMConvertToRadians(CAMERA_FOV_DEGREES),
		aspectRatio,
		CAMERA_NEAR_CLIP,
		CAMERA_FAR_CLIP
	);
}

/**
 * @brief デバイスロスト時の処理
 */
void GameplayScene::OnDeviceLost()
{
	m_basicEffect.reset();
	m_inputLayout.Reset();
}