/*****************************************************************//**
 * @file    CollisionSystem.cpp
 * @brief   プレイヤー・敵・弾丸・ミサイル間の衝突判定およびヒット時のエフェクト生成管理の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Collision/CollisionSystem.h"
#include "Game/World/World.h"
#include "Game/GameObjects/Enemy/EnemyManager.h"
#include "Game/GameObjects/Player/Player.h"
#include "Game/GameObjects/Enemy/Enemy.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include "Game/Camera/LockOnCamera.h"
#include "Game/Systems/Sound/SoundManager.h"

#include "Game/GameObjects/projectile/ProjectileManager.h"
#include "Game/GameObjects/Projectile/Bullet/Bullet.h"
#include "Game/GameObjects/projectile/Missile/CruiseMissile.h"

#include "Game/GameObjects/Robot/Control/WeaponController.h"
#include "Game/GameObjects/Robot/Robot.h"

/**
 * @brief コンストラクタ
 * @param[in] World エンティティ管理者へのポインタ
 */
CollisionSystem::CollisionSystem(World* World)
    : m_world(World)
    , m_isPlayerColliding(false)
{
}

/**
 * @brief 更新処理
 */
void CollisionSystem::Update()
{
    m_isPlayerColliding = false;

    if (!m_world) return;

    Player* player = m_world->GetPlayer();
    EnemyManager* enemyManager = m_world->GetEnemyManager();
    if (!player || !player->GetRobot() || !enemyManager) return;

    Robot* playerRobot = player->GetRobot();
    const auto& enemies = enemyManager->GetEnemies();

    // 全ての敵に対してループ処理を行う
    for (const auto& enemyPtr : enemies)
    {
        ProcessEnemyCollision(playerRobot, enemyPtr.get());
    }
}

/**
 * @brief 敵1体に対する衝突・攻撃判定の処理
 * @param[in] playerRobot プレイヤーのロボット
 * @param[in] enemy 判定対象の敵ポインタ
 */
void CollisionSystem::ProcessEnemyCollision(Robot* playerRobot, Enemy* enemy)
{
    // 敵が存在しない、あるいは死亡している場合はスキップ
    if (!enemy || !enemy->IsActive() || !enemy->GetRobot()) return;

    Robot* enemyRobot = enemy->GetRobot();

    // 体同士の判定 (Robot vs Robot)
    CheckBodyCollision(playerRobot, enemyRobot);

    // プレイヤーの武器 -> 敵への攻撃判定
    if (auto pWeapon = playerRobot->GetWeapon()) {
        CheckWeaponAttacks(pWeapon, enemyRobot);
    }

    // 敵の武器 -> プレイヤーへの攻撃判定
    if (auto enemyWeapon = enemyRobot->GetWeapon()) {
        CheckWeaponAttacks(enemyWeapon, playerRobot);
    }
}

/**
 * @brief 体同士の衝突判定
 * @param[in] robotA ロボットA
 * @param[in] robotB ロボットB
 */
void CollisionSystem::CheckBodyCollision(Robot* robotA, Robot* robotB)
{
    // スフィア（球）の判定データを取得
    DirectX::BoundingSphere sphereA = robotA->GetBoundingSphere();
    DirectX::BoundingSphere sphereB = robotB->GetBoundingSphere();

    // 球同士が重なっているかチェック
    if (sphereA.Intersects(sphereB))
    {
        m_isPlayerColliding = true;

        DirectX::SimpleMath::Vector3 posA = robotA->GetPosition();
        DirectX::SimpleMath::Vector3 posB = robotB->GetPosition();

        // 水平方向（XZ平面）の、中心同士の距離を計算
        DirectX::SimpleMath::Vector3 diff = posB - posA;
        diff.y = 0.0f;

        float dist = diff.Length();

        // 2つの球の半径の合計が、それ以上近づけない限界の距離
        float minDist = sphereA.Radius + sphereB.Radius;

        if (dist < minDist)
        {
            // 完全重なり（距離0）によるゼロ除算と反転を防ぐ
            DirectX::SimpleMath::Vector3 collisionNormal;
            if (dist < ZERO_TOLERANCE)
            {
                collisionNormal = DirectX::SimpleMath::Vector3::Right;
                dist = ZERO_TOLERANCE;
            }
            else
            {
                collisionNormal = diff / dist; // 球の中心を結ぶ、綺麗な反発方向
            }

            float overlap = minDist - dist; // めり込んでいる正確な深さ

            // 位置のめり込み解決
            ResolveOverlap(robotA, robotB, posA, posB, collisionNormal, overlap);

            // 速度ベクトルの調整
            AdjustVelocities(robotA, robotB, collisionNormal);
        }
    }
}

/**
 * @brief キャラクター同士のめり込み（位置）解決
 * @param[in] robotA ロボットA
 * @param[in] robotB ロボットB
 * @param[in] posA ロボットAの現在座標
 * @param[in] posB ロボットBの現在座標
 * @param[in] collisionNormal 反発方向の法線ベクトル
 * @param[in] overlap めり込みの深さ
 */
void CollisionSystem::ResolveOverlap(Robot* robotA, Robot* robotB, const DirectX::SimpleMath::Vector3& posA, 
    const DirectX::SimpleMath::Vector3& posB, const DirectX::SimpleMath::Vector3& collisionNormal, float overlap)
{
    // 座標のめり込みをスフィアの表面でストップさせる
    if (robotB->GetVelocity().LengthSquared() < ZERO_TOLERANCE)
    {
        // 相手（B：タレットなど速度0の固定物）なら、自分（A）だけを戻す（タレットは動かない）
        robotA->SetPosition(posA - collisionNormal * overlap);
    }
    else
    {
        // 動くキャラ同士（プレイヤー vs 敵）ならお互いを半分ずつ戻す
        robotA->SetPosition(posA - collisionNormal * (overlap * PUSH_AWAY_HALF_RATIO));
        robotB->SetPosition(posB + collisionNormal * (overlap * PUSH_AWAY_HALF_RATIO));
    }
}

/**
 * @brief 衝突時の速度ベクトルの調整
 * @param[in] robotA ロボットA
 * @param[in] robotB ロボットB
 * @param[in] collisionNormal 反発方向の法線ベクトル
 */
void CollisionSystem::AdjustVelocities(Robot* robotA, Robot* robotB,
    const DirectX::SimpleMath::Vector3& collisionNormal)
{
    // ぶつかった方向に向かっている速度成分だけを削る
    DirectX::SimpleMath::Vector3 velA = robotA->GetVelocity();
    float dotA = velA.Dot(collisionNormal);
    if (dotA > 0.0f)
    {
        robotA->SetVelocity(velA - (collisionNormal * dotA));
    }

    DirectX::SimpleMath::Vector3 velB = robotB->GetVelocity();
    float dotB = velB.Dot(-collisionNormal);
    if (dotB > 0.0f)
    {
        robotB->SetVelocity(velB - ((-collisionNormal) * dotB));
    }
}

/**
 * @brief 武器の攻撃 -> ロボットへの判定
 * @param[in] weapon 攻撃元の武器コントローラー
 * @param[in] target 判定対象のロボット
 */
void CollisionSystem::CheckWeaponAttacks(WeaponController* weapon, Robot* target)
{
    auto pProjectileMgr = weapon->GetProjectileManager();

    if (pProjectileMgr)
    {
        CheckBulletCollision(pProjectileMgr->GetProjectiles(), target);
    }
}

/**
 * @brief 爆発エフェクト生成
 * @param[in] pos 発生位置
 */
void CollisionSystem::SpawnExplosion(const DirectX::SimpleMath::Vector3& pos)
{
    // シングルトン経由でエフェクト発生
    EffectSystem::Instance()->SpawnExplosion(pos);
    SoundManager::Instance().PlaySE(L"SE_Explosion_Hit");
}