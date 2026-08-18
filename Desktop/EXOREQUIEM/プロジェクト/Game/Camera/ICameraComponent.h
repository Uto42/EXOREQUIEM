/*****************************************************************//**
 * @file    ICameraComponent.h
 * @brief   カメラ機能（追従、ロックオン等）を拡張するためのインターフェース
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

class Camera;

/**
 * @class ICameraComponent
 * @brief カメラ機能（追従、ロックオン等）を拡張するためのインターフェース
 */
class ICameraComponent
{
public:
	// デストラクタ
	virtual ~ICameraComponent() = default;

	// 各コンポーネント固有の毎フレーム更新処理
	virtual void Update(Camera& camera, float dt) = 0;
};