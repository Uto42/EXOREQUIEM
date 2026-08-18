/*****************************************************************//**
 * @file    WeaponBase.h
 * @brief   全武器共通の部品（Runtime）と処理を持つ基底クラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Weapon/IWeapon.h"
#include "Game/GameObjects/Weapon/WeaponRuntime.h"

/**
 * @class WeaponBase
 * @brief IWeaponを実装し、全武器共通の部品（Runtime）と処理（ターゲット決定）を持つ基底クラス
 */
class WeaponBase : public IWeapon
{
public:
    // コンストラクタでRuntimeの初期値を設定させる
    WeaponBase(int maxAmmo, float fireInterval, float reloadTime)
        : m_runtime(maxAmmo, fireInterval, reloadTime)
    {
    }
	// デストラクタ
    virtual ~WeaponBase() = default;
	// 更新処理
    virtual void Update(float dt) override { m_runtime.Update(dt); }

    // リロード処理の開始
    virtual void StartReload() override { m_runtime.StartReload(); }
    // 現在の残弾数を取得
    virtual int GetCurrentAmmo() const override { return m_runtime.GetCurrentAmmo(); }
    // 最大装弾数を取得
    virtual int GetMaxAmmo() const { return m_runtime.GetMaxAmmo(); }
    // リロード中かどうかを取得
    virtual bool IsReloading() const { return m_runtime.IsReloading(); }
    // リロード進行度の取得
    virtual float GetReloadTimeRate() const override { return m_runtime.GetReloadTimeRate(); }

protected:
    // 重複していたターゲット決定処理
    const Robot* DetermineTarget(const Robot* target, LockOnSystem* lockOnSystem, const Robot* manualTarget) const;

    // タイマー・残弾管理の部品
    WeaponRuntime m_runtime;
};