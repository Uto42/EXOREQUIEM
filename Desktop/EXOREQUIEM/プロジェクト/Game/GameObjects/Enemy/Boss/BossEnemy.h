/*****************************************************************//**
 * @file    BossEnemy.h
 * @brief   ボス専用のエネミークラスの宣言（Enemyクラスを拡張）
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Enemy/Enemy.h"

 // 通常のエネミー機能をベースに、上下分割モデルなどを扱うボス用クラス
class BossEnemy : public Enemy
{
public:
	// コンストラクタ
	BossEnemy();
	// デストラクタ
	virtual ~BossEnemy() override;

	// ボス専用の初期化（上下パーツの個別モデルロード等）
	void InitializeBoss(ID3D11Device* device, const DirectX::SimpleMath::Vector3& position,
		const wchar_t* lowerModelPath, const wchar_t* upperModelPath, const wchar_t* textureDir);
};