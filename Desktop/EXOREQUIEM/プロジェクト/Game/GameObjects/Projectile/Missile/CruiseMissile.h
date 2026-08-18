/*****************************************************************//**
 * @file    CruiseMissile.h
 * @brief   巡航ミサイルの誘導アルゴリズム実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Projectile/Missile/Missile.h"

 // ターゲットの未来位置を予測し、段階的な旋回で追尾する巡航ミサイルクラス
class CruiseMissile : public Missile
{
public:
	// コンストラクタ
	CruiseMissile();
	// デストラクタ
	virtual ~CruiseMissile() override;

private:
	// ターゲットの速度を考慮した未来の着弾予測座標（偏差射撃の目標点）を計算する
	DirectX::SimpleMath::Vector3 CalculateAimPoint(float dt) override;
	// 算出されたエイムポイントへ向かうための旋回計算と速度更新を行う
	void CalculateSteering(float dt, const DirectX::SimpleMath::Vector3& toTarget, float dot) override;
};