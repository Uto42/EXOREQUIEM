/*****************************************************************//**
 * @file    FreeCamera.h
 * @brief   PV撮影・デバッグ用フリーカメラの制御実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/
#pragma once

class FreeCamera
{
public:
	// コンストラクタ
	FreeCamera();
	// デストラクタ
	~FreeCamera() = default;
	// 初期化処理
	void Initialize(const DirectX::SimpleMath::Vector3& initialPos, float initialYaw, float initialPitch);
	// 更新処理
	void Update(float dt);

	// 描画用のビュー行列を取得
	DirectX::SimpleMath::Matrix GetViewMatrix() const;

private:
	// --- 定数パラメータ ---
	static constexpr float MIN_PITCH_DEG = -89.0f;                  //< ピッチ角の最小値（度）
	static constexpr float MAX_PITCH_DEG = 89.0f;                   //< ピッチ角の最大値（度）
	static constexpr float BASE_MOVE_SPEED = 50.0f;                 //< 基本移動速度
	static constexpr float BASE_ROT_SPEED = 2.0f;                   //< 基本回転速度
	static constexpr float SLOW_RATE_HORIZONTAL = 0.4f;             //< 減速時の水平移動倍率
	static constexpr float SLOW_RATE_VERTICAL = 0.1f;               //< 減速時の垂直移動倍率
	static constexpr float EPSILON_SQUARED = 0.001f;                //< ゼロ除算判定用の極小値（二乗値）
	static constexpr unsigned short KEY_IS_DOWN_MASK = 0x8000;      //< キー入力判定用ビットマスク

	DirectX::SimpleMath::Vector3 m_position;    //< カメラのワールド座標
	float m_yaw;                                //< ヨー角（Y軸回転）
	float m_pitch;                              //< ピッチ角（X軸回転）

	float m_minPitch;                           //< ピッチ角の下限（ラジアン）
	float m_maxPitch;                           //< ピッチ角の上限（ラジアン）
};