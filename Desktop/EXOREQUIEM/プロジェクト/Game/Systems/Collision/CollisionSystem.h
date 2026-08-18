/*****************************************************************//**
 * @file    CollisionSystem.h
 * @brief   プレイヤー・敵・弾丸・ミサイル間の衝突判定およびヒット時のエフェクト生成管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Effect/Visuals/DamageIndicator.h"
#include "Game/GameObjects/Robot/Robot.h"

 // 前方宣言
class World;
class WeaponController;
class Enemy;

// 衝突判定管理クラス
// キャラクター同士、武器とキャラクターの当たり判定を行う
class CollisionSystem
{
public:
    // コンストラクタ
    CollisionSystem(World* world);
    // 更新処理
    void Update();

    // 衝突結果の取得
    bool IsPlayerCollidingWithEnemy() const { return m_isPlayerColliding; }

private:
    // 敵1体に対する衝突・攻撃判定の処理
    void ProcessEnemyCollision(Robot* playerRobot, Enemy* enemy);

    // 体同士の衝突判定
    void CheckBodyCollision(Robot* robotA, Robot* robotB);

    // キャラクター同士のめり込み（位置）解決
    void ResolveOverlap(Robot* robotA, Robot* robotB,
        const DirectX::SimpleMath::Vector3& posA, const DirectX::SimpleMath::Vector3& posB,
        const DirectX::SimpleMath::Vector3& collisionNormal, float overlap);

    // 衝突時の速度ベクトルの調整
    void AdjustVelocities(Robot* robotA, Robot* robotB, const DirectX::SimpleMath::Vector3& collisionNormal);

    // ロボットへの攻撃判定
    void CheckWeaponAttacks(WeaponController* weapon, Robot* target);

private:
    // 被弾時の個別処理（ダメージ適用、エフェクト、演出呼び出し）
    template <typename BulletSmartPtr>
    void HandleBulletHit(Robot* target, const BulletSmartPtr& bullet)
    {
        // ダメージ適用
        target->TakeDamage(static_cast<float>(bullet->GetDamage()));

        // 弾を消去
        bullet->SetActive(false);

        // 被弾エフェクト生成
        this->SpawnExplosion(bullet->GetPosition());

        DirectX::SimpleMath::Vector3 actualShooterPos = bullet->GetShooterPosition();
        if (bullet->GetOwnerRobot())
        {
            actualShooterPos = bullet->GetOwnerRobot()->GetPosition();
        }

        // 被弾時の演出呼び出し（プレイヤーならカメラシェイク等、敵なら別の演出が走る）
        target->OnHit(actualShooterPos);
    }

    // 汎用的な衝突判定ロジック (弾丸 vs キャラクター)
    template <typename Container>
    void CheckBulletCollision(const Container& bullets, Robot* target)
    {
        if (!target) return;

        // ターゲットの当たり判定ボックスを取得
        DirectX::BoundingSphere targetSphere = target->GetBoundingSphere();

        for (const auto& bullet : bullets)
        {
            // アクティブでない弾は無視
            if (!bullet->IsActive()) continue;

            // 弾を撃ったロボットが自分なら衝突計算をスキップする
            if (bullet->GetOwnerRobot() == target) continue;

            // 当たり判定
            if (bullet->GetCollider().Intersects(targetSphere))
            {
                HandleBulletHit(target, bullet);
            }
        }
    }

    // 爆発エフェクト生成ヘルパー
    void SpawnExplosion(const DirectX::SimpleMath::Vector3& pos);

private:
    // --- 定数パラメータ ---
    static constexpr float ZERO_TOLERANCE = 0.001f;         //< 衝突判定の際のゼロ判定の許容値
    static constexpr float PUSH_AWAY_HALF_RATIO = 0.5f;     //< 衝突時の押し返し距離の半分

    World* m_world;                                         //< エンティティ管理者へのポインタ
    bool m_isPlayerColliding;                               //< プレイヤー衝突フラグ
};