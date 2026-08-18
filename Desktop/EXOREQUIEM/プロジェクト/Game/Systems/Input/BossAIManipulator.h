/*****************************************************************//**
 * @file    BossAIManipulator.h
 * @brief   ボスのAI制御（移動、攻撃パターンのフェーズ管理、壁避け）
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Input/IManipulator.h"

class Robot;

// ボス専用のAI制御クラス
class BossAIManipulator : public IManipulator
{
public:
	// コンストラクタ
	BossAIManipulator(Robot* owner, Robot* target);
	// デストラクタ
	virtual ~BossAIManipulator();

	// 毎フレーム呼ばれ、ボスの「行動入力」を決定する
	virtual RobotCommand GetCommand(float dt) override;

private:
	// ターゲットへの各種メトリクス（方向・距離・照準）を算出
	void UpdateTargetMetrics(RobotCommand& cmd, DirectX::SimpleMath::Vector3& outDirToTarget,
		DirectX::SimpleMath::Vector3& outTrueDirToTarget, float& outCurrentDistance) const;

	// HP割合によるフェーズ判定と行動サイクルのタイマー更新
	void UpdatePhaseAndCycle(RobotCommand& cmd, float dt, bool& outIsLowHP);

	// 現在のフェーズに応じた攻撃アクションの決定と目標距離の設定
	void DetermineAttackActions(RobotCommand& cmd, float currentDistance, bool isLowHP,
		float& outIdealDistance, float& outMoveSpeedMultiplier, bool& outIsTooClose) const;

	// ターゲットとの距離に基づき基本となる移動方向（接近・離反・周回）を算出
	void DetermineMovement(DirectX::SimpleMath::Vector3& outFinalDir, DirectX::SimpleMath::Vector3& outTangent,
		const DirectX::SimpleMath::Vector3& dirToTarget, float currentDistance, float idealDistance, bool isTooClose) const;

	// 進行方向の障害物を検知し壁を避ける方向に移動ベクトルを補正
	void AvoidObstacles(RobotCommand& cmd, DirectX::SimpleMath::Vector3 finalDir, DirectX::SimpleMath::Vector3 tangent,
		const DirectX::SimpleMath::Vector3& dirToTarget, float moveSpeedMultiplier, bool isTooClose);

private:
	// --- 定数パラメータ ---
	// 行動フェーズの秒数
	static constexpr float PHASE_DURATION_MAIN_WEAPON = 4.0f;            //< メイン武器フェーズの継続時間
	static constexpr float PHASE_DURATION_MISSILE = 2.0f;                //< ミサイルフェーズの継続時間
	static constexpr float PHASE_DURATION_COOLDOWN = 1.5f;               //< クールダウンフェーズの継続時間

	// 間合い（距離）の基準値
	static constexpr float DISTANCE_IDEAL_SHOTGUN = 16.0f;               //< ショットガンの理想距離
	static constexpr float DISTANCE_TOO_CLOSE_SHOTGUN = 12.0f;           //< ショットガン時の近すぎ判定距離
	static constexpr float DISTANCE_IDEAL_MACHINEGUN = 35.0f;            //< マシンガンの理想距離
	static constexpr float DISTANCE_TOO_CLOSE_MACHINEGUN = 20.0f;        //< マシンガン時の近すぎ判定距離
	static constexpr float DISTANCE_IDEAL_COOLDOWN = 40.0f;              //< 前半戦クールダウン時の理想距離

	// 判定・移動補正の係数
	static constexpr float HP_THRESHOLD_RATIO = 0.5f;                    //< フェーズ変化の基準となるHP割合（50%）
	static constexpr float EPSILON_SQ = 0.001f;                          //< ゼロベクトル判定用の微小値二乗
	static constexpr float APPROACH_FORCE_MULTIPLIER = 0.1f;             //< 接近力の算出係数
	static constexpr float ESCAPE_FORCE_VALUE = -1.0f;                   //< 緊急後退時の接近力（マイナス値）
	static constexpr float MAX_APPROACH_FORCE = 1.0f;                    //< 接近力の最大クランプ値
	static constexpr float MIN_APPROACH_FORCE = -1.0f;                   //< 接近力の最小クランプ値
	static constexpr float TANGENT_BLEND_NORMAL = 0.5f;                  //< 通常移動時の横移動ブレンド率
	static constexpr float TANGENT_BLEND_EVADE = 0.8f;                   //< 壁避け時の横移動ブレンド率

	// 障害物センサーの設定
	static constexpr float SENSOR_HEIGHT_OFFSET = 1.0f;                  //< センサーの高さオフセット
	static constexpr float SENSOR_RAY_LENGTH = 5.0f;                     //< センサーのレイの長さ

	// 周回方向
	static constexpr float DIRECTION_RIGHT = 1.0f;                       //< 右回りの方向値
	static constexpr float DIRECTION_LEFT = -1.0f;                       //< 左回りの方向値
	static constexpr float REVERSE_DIRECTION = -1.0f;                    //< 方向反転用の係数

private:
	// --- クラス内部変数 ---
	Robot* m_owner;                                                      //< ボス自身
	Robot* m_target;                                                     //< プレイヤー

	float m_attackTimer;                                                 //< フェーズ管理用タイマー
	float m_circleDirection;                                             //< 回り込む方向

	bool m_isPhase2 = false;                                             //< ボスのフェーズ2フラグ
};