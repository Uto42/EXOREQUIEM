/*****************************************************************//**
 * @file    IWeapon.h
 * @brief   全武器共通のインターフェースクラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

// 前方宣言
class Robot;
class ProjectileManager;
class LockOnSystem;

/**
 * @class IWeapon
 * @brief 全武装共通のインターフェース
 */
class IWeapon
{
public:
    // デストラクタ
    virtual ~IWeapon() = default;

    // 更新処理
    virtual void Update(float dt) = 0;
    //リロード処理の開始
    virtual void StartReload() = 0;
    // 現在の弾数の取得
    virtual int GetCurrentAmmo() const = 0;
    // リロード進行度の取得
    virtual float GetReloadTimeRate() const = 0;

    // 発射関数
    virtual void TryFire(
        const DirectX::SimpleMath::Vector3& robotPos,
        const DirectX::SimpleMath::Quaternion& robotRot,
        const DirectX::SimpleMath::Vector3& aimPosOrDir,
        const Robot* owner,
        const Robot* target,
        ProjectileManager* projectileManager,
        LockOnSystem* lockOnSystem,
        const Robot* manualTarget
    ) = 0;
};