/*****************************************************************//**
 * @file    DummyAIManipulator.cpp
 * @brief   チュートリアル用の何もしないダミーAI制御の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Input/DummyAIManipulator.h"

/**
 * @brief AIによる行動コマンドの生成（ダミー用）
 * @param[in] dt 前フレームからの経過時間
 * @return RobotCommand 全てのアクションがOFFになった空のコマンド
 */
RobotCommand DummyAIManipulator::GetCommand(float dt)
{
    // 使わない引数の警告消し
    UNREFERENCED_PARAMETER(dt);

    RobotCommand cmd;

    // 移動・視線・射撃方向をゼロに
    cmd.moveDirection = DirectX::SimpleMath::Vector3::Zero;
    cmd.lookDirection = DirectX::SimpleMath::Vector3::Zero;
    cmd.aimDirection = DirectX::SimpleMath::Vector3::Zero;

    // 全てのアクション入力をOFFに
    cmd.jump = false;
    cmd.evade = false;
    cmd.rise = false;
    cmd.fireGun = false;
    cmd.fireMissile = false;

    // 何もしないコマンドをそのまま返す
    return cmd;
}