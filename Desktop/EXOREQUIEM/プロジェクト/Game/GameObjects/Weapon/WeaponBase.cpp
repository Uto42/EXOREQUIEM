/*****************************************************************//**
 * @file    WeaponBase.cpp
 * @brief   全武器共通の部品（Runtime）と処理を持つ基底クラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Weapon/WeaponBase.h"
#include "Game/Systems/LockOn/LockOnSystem.h"

/**
 * @brief 攻撃対象（ターゲット）を決定する
 * @param[in] target 直接指定されたターゲット
 * @param[in] lockOnSystem ロックオンシステム
 * @param[in] manualTarget 手動で指定されたターゲット
 * @return const Robot* 最終的に決定されたターゲット（対象がいない場合は nullptr）
 */
const Robot* WeaponBase::DetermineTarget(const Robot* target, LockOnSystem* lockOnSystem, const Robot* manualTarget) const
{
    // 直接指定があればそれを優先
    if (target)
    {
        return target;
    }

    // ロックオンシステムが有効、かつ「UIの照準が画面内の敵に乗っている」場合
    if (lockOnSystem && lockOnSystem->GetCurrentTarget() && lockOnSystem->IsTargetOnScreen())
    {
        return lockOnSystem->GetCurrentTarget();
    }

    // 手動ターゲット
    if (manualTarget)
    {
        return manualTarget;
    }

    return nullptr;
}