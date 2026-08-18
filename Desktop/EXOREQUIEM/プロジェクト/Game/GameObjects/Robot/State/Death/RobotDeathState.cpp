/*****************************************************************//**
 * @file    RobotDeathState.cpp
 * @brief   ロボットの死亡状態（爆発演出から消滅まで）の挙動を管理するクラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/State/Death/RobotDeathState.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Systems/Sound/SoundManager.h"
#include <random>

/**
 * @brief 死亡状態開始時の初期化
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotDeathState::Enter(Robot* robot)
{
    UNREFERENCED_PARAMETER(robot);

    m_timer = 0.0f;
    m_explosionTimer = 0.0f;
    m_finalExplosionTriggered = false;
    m_explosionCount = 0;
}

/**
 * @brief 死亡演出の更新処理
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] dt 前フレームからの経過時間
 * @return RobotStateTypes 常に死亡状態を継続
 */
RobotStateTypes RobotDeathState::Update(Robot* robot, const RobotCommand& cmd, float dt)
{
	UNREFERENCED_PARAMETER(cmd);

    // 大爆発が既に実行されている場合は更新を行わない
    if (m_finalExplosionTriggered) return RobotStateTypes::Death;

    m_timer += dt;

    if (m_timer < SMALL_EXPLOSION_END_TIME)
    {
        // 一定時間経過まで小爆発演出を行う
        UpdateSmallExplosion(robot, dt);
    }
    else if (m_timer >= FINAL_EXPLOSION_START_TIME)
    {
        // 最終的な大爆発演出を実行
        ExecuteFinalDeath(robot);
    }

    return RobotStateTypes::Death;
}

/**
 * @brief 断続的に発生する小爆発の更新
 * @param[in] robot 対象のロボットオブジェクト
 * @param[in] dt 前フレームからの経過時間
 */
void RobotDeathState::UpdateSmallExplosion(Robot* robot, float dt)
{
    if (m_explosionCount >= MAX_SMALL_EXPLOSION_COUNT) return;

	//--- 爆発タイマーの更新と爆発の発生判定 ---
    m_explosionTimer += dt;

    if (m_explosionTimer > SMALL_EXPLOSION_INTERVAL)
    {
        m_explosionTimer = 0.0f;
        m_explosionCount++;

		// ランダムなオフセットを生成して爆発位置を決定
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-SMALL_EXPLOSION_OFFSET_RANGE, SMALL_EXPLOSION_OFFSET_RANGE);

		// 爆発の中心位置を取得 
        DirectX::SimpleMath::Vector3 pPos = GetExplosionCenter(robot);

		// 機体の大きさに応じて爆発の拡散範囲を調整
        float sizeScale = std::max(1.0f, robot->GetAimPosition().y - robot->GetPosition().y);

        DirectX::SimpleMath::Vector3 offset(dist(gen), dist(gen), dist(gen));

        offset *= sizeScale;

		// エフェクトの生成とサウンド再生
        EffectSystem::Instance()->SpawnExplosion(pPos + offset);
        SoundManager::Instance().PlaySE(L"SE_Explosion_Defeat");
    }
}

/**
 * @brief 最終的な大爆発演出と機体の非表示化処理
 * @param[in] robot 対象のロボットオブジェクト
 */
void RobotDeathState::ExecuteFinalDeath(Robot* robot)
{
    m_finalExplosionTriggered = true;

	// ランダムなオフセットを生成して爆発位置を決定
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-FINAL_EXPLOSION_OFFSET_RANGE, FINAL_EXPLOSION_OFFSET_RANGE);

	// 爆発の中心位置を取得
    DirectX::SimpleMath::Vector3 pPos = GetExplosionCenter(robot);

	// 機体の大きさに応じて爆発の拡散範囲を調整
    float sizeScale = std::max(1.0f, robot->GetAimPosition().y - robot->GetPosition().y);

    // 多数のエフェクトを拡散させて大爆発を表現
    for (int i = 0; i < FINAL_EXPLOSION_EFFECT_COUNT; ++i)
    {
        DirectX::SimpleMath::Vector3 spread(dist(gen), dist(gen), dist(gen));
        spread *= sizeScale;
        EffectSystem::Instance()->SpawnExplosion(pPos + spread);
    }

	// 機体の非表示化と非アクティブ化
    robot->SetVisible(false);
    robot->SetActive(false);
    SoundManager::Instance().PlaySE(L"SE_Explosion_Defeat");
}

/**
 * @brief 爆発を発生させる機体中心位置の計算
 * @param[in] robot 対象のロボットオブジェクト
 * @return Vector3 計算された中心位置
 */
DirectX::SimpleMath::Vector3 RobotDeathState::GetExplosionCenter(Robot* robot) const
{
    return robot->GetAimPosition();
}

/**
 * @brief 死亡状態終了時の処理
 * @param[in] robot ロボットオブジェクト
 */
void RobotDeathState::Exit(Robot* robot)
{
    UNREFERENCED_PARAMETER(robot);
}