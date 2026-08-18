/*****************************************************************//**
 * @file    BossEnemy.cpp
 * @brief   ボス専用のエネミークラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Enemy/Boss/BossEnemy.h"
#include "Game/GameObjects/Robot/Boss/BossRobot.h"

/**
 * @brief コンストラクタ
 */
BossEnemy::BossEnemy()
{
	// 親クラス（Enemy）で生成される標準ロボットのインスタンスを、ボス専用ロボットへ差し替える
	m_robot = std::make_shared<BossRobot>();
}

/**
 * @brief デストラクタ
 */
BossEnemy::~BossEnemy()
{
}

/**
 * @brief ボス専用の初期化処理
 * @param[in] device D3D11デバイス
 * @param[in] position 初期配置座標
 * @param[in] lowerModelPath 下半身3Dモデルのファイルパス
 * @param[in] upperModelPath 上半身3Dモデルのファイルパス
 * @param[in] textureDir テクスチャの格納ディレクトリパス
 */
void BossEnemy::InitializeBoss(ID3D11Device* device, const DirectX::SimpleMath::Vector3& position,
	const wchar_t* lowerModelPath, const wchar_t* upperModelPath, const wchar_t* textureDir)
{
	// 共通のエネミー初期化処理を実行（座標設定や下半身モデルの読み込み等）
	Enemy::Initialize(device, position, true, lowerModelPath, textureDir);

	// ボス専用のインスタンスへキャストし、上半身モデルを追加ロードする
	auto bossRobot = std::dynamic_pointer_cast<BossRobot>(m_robot);
	if (bossRobot)
	{
		bossRobot->LoadUpperModel(device, upperModelPath, textureDir);
	}
}