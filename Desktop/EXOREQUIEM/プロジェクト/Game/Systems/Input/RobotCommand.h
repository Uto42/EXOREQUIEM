/*****************************************************************//**
 * @file    RobotCommand.h
 * @brief   コントローラーからの入力を機体（Robot）へ伝えるための命令構造体
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

/**
 * @struct RobotCommand
 * @brief  移動、アクション、攻撃などの操作入力を一括で保持するデータ構造
 */
struct RobotCommand
{
    // --- 移動関連 ---
    DirectX::SimpleMath::Vector3 moveDirection = {};    //< 入力された移動方向
    bool jump = false;                                  //< ジャンプ入力
    bool rise = false;
    bool evade = false;                                 //< 回避入力

    // --- 攻撃関連 ---
	bool fireGun = false;                               //< マシンガン射撃入力  
	bool fireMissile = false;                           //< ミサイル発射入力
    bool fireShotgun = false;                           //< ショットガン射撃入力
    bool fireHighAltitudeMissile = false;               //< 高高度ミサイル発射入力
	bool reload = false;								//< リロード入力
    bool switchWeapon = false;                          //< 武器セット切り替え入力

    // --- 照準関連 ---
    DirectX::SimpleMath::Vector3 aimDirection = {};     //< 攻撃目標または照準方向
	DirectX::SimpleMath::Vector3 aimOriginPosition = {};//< 照準の基準位置
	DirectX::SimpleMath::Vector3 lookDirection = {};	//< 機体が「向きたい方向」のベクトル

    void Clear()
    {
        moveDirection = DirectX::SimpleMath::Vector3::Zero;

        fireGun = false;
        fireMissile = false;
        jump = false;
        rise = false;
        evade = false;
        reload = false;
        switchWeapon = false;
    }
};