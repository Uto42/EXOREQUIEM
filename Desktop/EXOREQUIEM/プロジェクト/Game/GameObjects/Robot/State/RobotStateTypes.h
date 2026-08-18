/*****************************************************************//**
 * @file    RobotStateTypes.h
 * @brief   ロボットのすべての状態（ステート）を定義する列挙型
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

/**
 * @enum RobotStateTypes
 * @brief ロボットのすべての状態を定義する列挙型
 */
enum class RobotStateTypes
{
    Idle,       //< 待機
    Moving,     //< 移動
    Jump,       //< ジャンプ
    Rise,       //< 上昇
    Evade,      //< 回避
    Death       //< 死亡
};