/*****************************************************************//**
 * @file    TopAttackMissile.h
 * @brief   高高度誘導ミサイル（トップアタック）の誘導アルゴリズム実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Projectile/Missile/Missile.h"

 // ターゲットの頭上高くへ上昇した後、急降下して上面装甲を狙うトップアタック式ミサイル
class TopAttackMissile : public Missile
{
public:
	// コンストラクタ
	TopAttackMissile();
	// デストラクタ
	virtual ~TopAttackMissile() override;
	// 初期化処理
	void Initialize(
		const DirectX::SimpleMath::Vector3& startPos,
		const DirectX::SimpleMath::Vector3& startDir,
		std::function<DirectX::SimpleMath::Vector3()> getPosFunc,
		std::function<DirectX::SimpleMath::Vector3()> getVelFunc,
		std::function<bool()> isActiveFunc,
		float speed,
		float turnSpeed,
		float lifeTime,
		const Robot* owner
	) override;

private:
	// ターゲットの上空（巡航高度）または直下（終末誘導）のエイム座標を計算する
	DirectX::SimpleMath::Vector3 CalculateAimPoint(float dt) override;
	// 巡航フェーズと終末フェーズに応じた旋回・加速処理を行う
	void CalculateSteering(float dt, const DirectX::SimpleMath::Vector3& toTarget, float dot) override;
	// 進行方向にあわせた姿勢更新（真下を向いた際のジンバルロック対策を含む）
	void UpdateRotationFromVelocity() override;

private:
	// --- 調整用定数パラメータ ---
	static constexpr float CRUISE_HEIGHT_OFFSET = 30.0f;          //< 巡航時の目標高度オフセット
	static constexpr float MAX_CRUISE_HEIGHT = 40.0f;             //< 最大巡航高度
	static constexpr float DIVE_TRIGGER_DISTANCE = 4.0f;          //< 降下攻撃を開始する距離
	static constexpr float TERMINAL_DIVE_SPEED_MULTIPLIER = 1.5f; //< 最終降下時の速度ブースト倍率
	static constexpr float GIMBAL_LOCK_THRESHOLD = 0.98f;         //< ジンバルロック回避の閾値内積

	// --- 状態値・パラメータ ---
	DirectX::SimpleMath::Vector3 m_terminalTargetPos;             //< 最終降下を開始した瞬間に確定する目標座標
	bool m_isTerminalPhase = false;                               //< 最終降下（ターミナル）フェーズへの移行フラグ
};