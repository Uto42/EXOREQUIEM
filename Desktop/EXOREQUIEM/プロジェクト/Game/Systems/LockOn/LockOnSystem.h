/*****************************************************************//**
 * @file    LockOnSystem.h
 * @brief   ロックオン制御システム：複数ターゲットからの最適対象選出、遮蔽物判定、および注視モードの管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

class Robot;
class StageManager;

/**
 * @class LockOnSystem
 * @brief ソフトロックとハードロックを管理し、最適なターゲットを自動選定するシステム
 */
class LockOnSystem
{
public:
	// コンストラクタ
	LockOnSystem();
	// デストラクタ
	~LockOnSystem();
	// 初期化処理
	void Initialize(Robot* playerRobot);
	// 更新処理
	void Update(const std::vector<Robot*>& targets,
		const DirectX::SimpleMath::Vector3& cameraForward, float mouseDeltaX, float mouseDeltaY, float dt);

	// ハードロックモードの切り替え
	void ToggleHardLock();
	// マウスの入力方向に基づき現在のターゲットの隣にいる敵を探す
	Robot* FindNextTargetInDirection(const std::vector<Robot*>& targets,
		const DirectX::SimpleMath::Vector3& cameraForward, float mouseDeltaX) const;

	// 現在のロックオン対象取得
	Robot* GetCurrentTarget() const { return m_isLockOnActive ? m_currentTarget : nullptr; }
	// ロックオン有効状態取得
	bool IsLockOnActive() const { return m_isLockOnActive; }
	// ハードロックモード状態取得
	bool IsHardLockMode() const { return m_isHardLockMode; }
	// ターゲットが画面内に収まっているかのフラグ取得
	bool IsTargetOnScreen() const { return m_isTargetOnScreen; }

	// ステージマネージャーの登録
	void SetStageManager(StageManager* sm) { m_stageManager = sm; }
	// ターゲットが画面内に収まっているかのフラグ設定
	void SetTargetOnScreen(bool isOnScreen) { m_isTargetOnScreen = isOnScreen; }

private:
	// --- ロックオン判定用補助関数 ---
	// 現在のターゲットがロック維持条件を満たしているか検証
	bool IsCurrentTargetValid(const DirectX::SimpleMath::Vector3& cameraForward) const;
	// ロックオン可能判定（視界・遮蔽物チェック）
	bool CanLockOnToTarget(Robot* target, const DirectX::SimpleMath::Vector3& cameraForward) const;
	// 最適なターゲットを検索
	Robot* FindBestTarget(const std::vector<Robot*>& targets, const DirectX::SimpleMath::Vector3& cameraForward) const;
	// ターゲット優先度のスコア計算
	float CalcScore(Robot* target, const DirectX::SimpleMath::Vector3& cameraForward) const;

private:
	// --- 定数パラメータ ---
	static constexpr float MAX_LOCK_ON_DISTANCE = 400.0f;           //< ロックオン限界距離
	static constexpr float LIMIT_ANGLE = 0.7f;                      //< 視野角制限（ドット積）
	static constexpr float HARD_LOCK_BIAS = 0.15f;                  //< ハードロック時のスコアボーナス
	static constexpr float RAYCAST_OFFSET_Y = 1.0f;                 //< レイキャスト時の上下オフセット（胸元の高さ）
	static constexpr float MIN_RAYCAST_DISTANCE = 0.01f;            //< レイキャストを行う最小距離閾値
	static constexpr float WEIGHT_DOT = 0.7f;                       //< スコア計算：画面中央（正面度）への重み（70%）
	static constexpr float WEIGHT_DISTANCE = 0.3f;                  //< スコア計算：距離の近さへの重み（30%）
	static constexpr float ZERO_HEALTH = 0.0f;                      //< 体力の死亡基準値
	static constexpr float INITIAL_MAX_SCORE = -1.0f;               //< 最適ターゲット検索時のスコア初期値
	static constexpr float MAX_RATIO = 1.0f;                        //< 割合の最大値 (100%)
	static constexpr float HARD_LOCK_RANGE_SCALE = 1.5f;            //< ハードロック時のロック維持距離の倍率
	static constexpr float MOUSE_INPUT_THRESHOLD = 0.1f;            //< ターゲット切り替えを検知するマウス入力閾値
	static constexpr float INITIAL_MIN_ANGLE = 9999.0f;             //< 隣接ターゲット検索時の角度差初期値

	// --- メンバ変数 ---
	Robot* m_playerRobot;                                           //< プレイヤーへの参照
	Robot* m_currentTarget;                                         //< 現在のターゲット
	StageManager* m_stageManager;                                   //< 地形判定用マネージャー
	bool m_isLockOnActive;                                          //< ロックオン成功中フラグ
	bool m_isHardLockMode;                                          //< ハードロック（強制追従）モード
	bool m_isTargetOnScreen = false;                                //< ターゲットが画面内にいるかどうかのフラグ
};