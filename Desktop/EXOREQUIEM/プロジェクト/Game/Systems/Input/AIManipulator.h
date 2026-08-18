/*****************************************************************//**
 * @file    AIManipulator.h
 * @brief   ロボットの汎用AI制御（間合い管理、アクション判断、視線チェック）
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Input/IManipulator.h"
#include "Game/Systems/Input/RobotCommand.h"
#include <random>

class Robot;
class StageManager;

class AIManipulator : public IManipulator
{
private:
	// AIの状態定義
	enum class AIBrainState
	{
		Combat     //< 戦闘中
	};

public:
	// コンストラクタ
	AIManipulator(Robot* pawn, const Robot* target, StageManager* stageManager);
	// デストラクタ
	virtual ~AIManipulator() = default;

	// 最新の入力コマンドを取得
	virtual RobotCommand GetCommand(float dt) override;

private:
	// 戦闘ロジックの更新
	void UpdateCombatLogic(RobotCommand& cmd, 
		const DirectX::SimpleMath::Vector3& dirToTarget, float dist, float heightDiff, float dt);

	// ターゲットへの射線が通っているか確認
	bool CheckLineOfSight(const DirectX::SimpleMath::Vector3& direction, float distance) const;

	// ターゲットに関する各種メトリクス（方向・距離・照準など）の計算
	void CalculateTargetMetrics(DirectX::SimpleMath::Vector3& outDirToTarget, float& outDist,
		float& outHeightDiff, DirectX::SimpleMath::Vector3& outAimDir) const;

	// 戦闘時における基本的な移動方針の決定と射撃制御
	void DetermineCombatMovement(RobotCommand& cmd, bool canSeePlayer,
		const DirectX::SimpleMath::Vector3& dirToTarget, float dist, float dt);

	// 進行方向の障害物を検知して迂回ルートを探す
	void PerformObstacleAvoidance(DirectX::SimpleMath::Vector3& ioMoveDirection) const;

	// アクション（回避・ジャンプ・上昇）の総合更新
	void UpdateActionDecisions(RobotCommand& cmd, float heightDiff, float dt);

	// 回避の意思決定
	void DecideEvade(RobotCommand& cmd, float dt);
	// ジャンプの意思決定
	void DecideJump(RobotCommand& cmd, float heightDiff, float dt);
	// 上昇の意思決定
	void DecideRise(RobotCommand& cmd, float heightDiff, float dt);

private:
	// --- 調整用定数パラメータ ---
	// 間合い・高度の閾値
	static constexpr float OPTIMAL_DISTANCE = 40.0f;           //< 適正距離
	static constexpr float DISTANCE_DEAD_ZONE = 5.0f;          //< 前後移動を停止する遊びの幅
	static constexpr float JUMP_HEIGHT_THRESH = 2.0f;          //< ジャンプ率が上がる高度差
	static constexpr float RISE_HEIGHT_THRESH = 5.0f;          //< ブースト上昇を行う最低高度差

	// 意思決定タイマーサイクル
	static constexpr float EVADE_TIMER_BASE = 1.0f;            //< 回避のベース時間（秒）
	static constexpr float EVADE_TIMER_RAND = 1.5f;            //< 回避のランダム幅（秒）
	static constexpr float JUMP_TIMER_BASE = 1.5f;             //< ジャンプのベース時間（秒）
	static constexpr float JUMP_TIMER_RAND = 1.5f;             //< ジャンプのランダム幅（秒）
	static constexpr float RISE_TIMER_BASE = 3.0f;             //< 上昇のベース時間（秒）
	static constexpr float RISE_TIMER_RAND = 2.0f;             //< 上昇のランダム幅（秒）

	// 行動確率
	static constexpr float JUMP_PROB_HIGH = 0.8f;              //< ターゲットが高い時のジャンプ確率
	static constexpr float JUMP_PROB_NORMAL = 0.4f;            //< 通常時のジャンプ確率

	// 各種高さ・オフセット
	static constexpr float PAWN_MUZZLE_Y = 1.0f;               //< 自身の銃口位置Yオフセット
	static constexpr float TARGET_BODY_Y = 1.0f;               //< 敵の狙う位置Yオフセット
	static constexpr float EYE_HEIGHT = 1.5f;                  //< 視線チェックの始点高さ
	static constexpr float SENSOR_HEIGHT_OFFSET = 1.0f;        //< 障害物回避センサーの始点Yオフセット
	static constexpr float SENSOR_RAY_LENGTH = 3.0f;           //< 障害物回避センサーの検知距離

	// 初期タイマー値（AI開始直後の行動を遅らせるため）
	static constexpr float INITIAL_JUMP_TIMER = 1.0f;          //< ジャンプの初期タイマー
	static constexpr float INITIAL_EVADE_TIMER = 1.0f;         //< 回避の初期タイマー
	static constexpr float INITIAL_RISE_TIMER = 2.0f;          //< 上昇の初期タイマー

	// 左右移動（ストレイフ）サイクル
	static constexpr float STRAFE_CYCLE = 4.0f;                //< 1周期の時間（秒）
	static constexpr float STRAFE_SWITCH_TIME = 2.0f;          //< 折り返し時間（秒）
	static constexpr float STRAFE_FORWARD_BLEND_RATIO = 0.5f;  //< 視線が通っていない時の直進と横移動のブレンド率

	// 射撃サイクル管理
	static constexpr float FIRE_BURST_TIME = 3.0f;             //< 連射する時間（秒）
	static constexpr float FIRE_REST_TIME = 2.0f;              //< 射撃を休止する時間（秒）
	static constexpr float FIRE_CYCLE_TOTAL = FIRE_BURST_TIME + FIRE_REST_TIME; //< 射撃の合計サイクル時間

	// --- 安全・バグ防止用の閾値 ---
	static constexpr float NORMALIZE_EPSILON = 0.001f;         //< ゼロ除算防止用の微小値

private:
	Robot* m_pawn;                                             //< 操作対象の機体
	const Robot* m_target;                                     //< 追跡対象の機体
	StageManager* m_stageManager;                              //< ステージ管理者

	AIBrainState m_brainState;                                 //< 現在の思考状態

	// 意思決定用タイマー
	float m_decisionTimer_Jump;                                //< ジャンプ判断用
	float m_decisionTimer_Evade;                               //< 回避判断用
	float m_decisionTimer_Rise;                                //< 上昇判断用
	float m_strafeTimer;                                       //< 左右移動（ストレイフ）の周期管理用
	float m_fireIntervalTimer = 0.0f;						   //< 射撃のインターバルを管理するタイマー

	std::mt19937 m_randGen;                                    //< 乱数生成器
};