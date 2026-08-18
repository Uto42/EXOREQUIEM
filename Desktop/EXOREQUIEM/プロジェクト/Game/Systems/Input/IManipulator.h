/*****************************************************************//**
 * @file    IManipulator.h
 * @brief   入力デバイス（キーボード）やAIからの操作命令を抽象化するインターフェース
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Systems/Input/RobotCommand.h"

 /**
  * @class IManipulator
  * @brief プレイヤーやAIの操作をRobotCommandとして抽出するためのインターフェース
  */
class IManipulator
{
public:
    // 仮想デストラクタ
    virtual ~IManipulator() = default;

    // 1フレーム分の操作コマンドを取得
    virtual RobotCommand GetCommand(float dt) = 0;

    // カメラ方向を受け取る関数
    virtual void SetCameraOrientation(const DirectX::SimpleMath::Vector3& forward, const DirectX::SimpleMath::Vector3& right) 
    {
		UNREFERENCED_PARAMETER(forward);
		UNREFERENCED_PARAMETER(right);
    }

	// カメラ位置を受け取る関数
    virtual void SetCameraPosition(const DirectX::SimpleMath::Vector3& position)
    {
        UNREFERENCED_PARAMETER(position);
    }
};