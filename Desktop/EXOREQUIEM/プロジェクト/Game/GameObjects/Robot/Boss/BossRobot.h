/*****************************************************************//**
 * @file    BossRobot.h
 * @brief   上下半身が独立して可動するボス用ロボットクラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/GameObjects/Robot/Robot.h"

 // 下半身の移動と独立して上半身がプレイヤーを追尾するボス機体
class BossRobot : public Robot
{
public:
	// コンストラクタ
	BossRobot();
	// デストラクタ
	virtual ~BossRobot() override;
	// ボスモデルの描画
	void Render(ID3D11DeviceContext* context, DirectX::CommonStates* states, 
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, 
		float alpha = 1.0f) override;

	// 上半身モデルの読み込み
	void LoadUpperModel(ID3D11Device* device, const wchar_t* upperModelPath, const wchar_t* textureDir);

	// 上下半身の向きの設定
	void SetRotation(const DirectX::SimpleMath::Quaternion& rot) override;
	// 上下半身の旋回更新
	void UpdateRotation(const RobotCommand& cmd, float dt) override;

	// 照準位置の取得
	DirectX::SimpleMath::Vector3 GetAimPosition() const override
	{
		return m_position + DirectX::SimpleMath::Vector3(0.0f, AIM_HEIGHT_OFFSET, 0.0f);
	}

private:
	// --- 調整用定数パラメータ ---
	static constexpr float AIM_HEIGHT_OFFSET = 7.0f;             //< 敵やミサイルから狙われる際の照準高さオフセット
	static constexpr float UPPER_TURN_SPEED = 2.0f;              //< 上半身の旋回速度（重量感を出すための係数）
	static constexpr float UPPER_RESET_TURN_SPEED = 5.0f;        //< ターゲットを見失った際の正面復帰速度
	static constexpr float INPUT_DEADZONE_SQUARED = 0.001f;      //< 旋回入力のデッドゾーン閾値
	static constexpr float ALPHA_VISIBLE_THRESHOLD = 0.01f;      //< 描画をスキップする透明度の閾値
	static constexpr float ALPHA_OPAQUE_THRESHOLD = 0.99f;       //< 不透明描画と判定する透明度の閾値

	// --- 状態値・パラメータ ---
	DirectX::SimpleMath::Quaternion m_upperRotation;             //< 上半身の現在の姿勢（回転）
	float m_upperHeightOffset = 0.0f;                            //< 上半身の高さ

	// --- リソース ---
	std::unique_ptr<DirectX::Model> m_upperModel;                //< 上半身モデル
};