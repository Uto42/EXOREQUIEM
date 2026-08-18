/*****************************************************************//**
 * @file    FreeCamera.cpp
 * @brief   PV撮影・デバッグ用フリーカメラの制御実装
 * 
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Camera/FreeCamera.h"

/**
 * @brief 値を特定の範囲内にクランプするヘルパー関数
 * @param v 対象の値
 * @param lo 最小値
 * @param hi 最大値
 * @return クランプされた値
 */
static constexpr float clamp_val(float v, float lo, float hi)
{
	return (v < lo) ? lo : (hi < v) ? hi : v;
}

/**
 * @brief コンストラクタ
 * @details ピッチ角の制限（度数法からラジアンへの変換）を初期化する
 */
FreeCamera::FreeCamera()
	: m_position(DirectX::SimpleMath::Vector3::Zero)
	, m_yaw(0.0f)
	, m_pitch(0.0f)
{
	//--- ピッチ角の制限をラジアンに変換して初期化 ---
	m_minPitch = DirectX::XMConvertToRadians(MIN_PITCH_DEG);
	m_maxPitch = DirectX::XMConvertToRadians(MAX_PITCH_DEG);
}

/**
 * @brief メインカメラから切り替わった際に、座標と角度を引き継ぐ
 * @param initialPos 初期座標
 * @param initialYaw 初期ヨー角
 * @param initialPitch 初期ピッチ角
 */
void FreeCamera::Initialize(const DirectX::SimpleMath::Vector3& initialPos, float initialYaw, float initialPitch)
{
	m_position = initialPos;
	m_yaw = initialYaw;
	m_pitch = initialPitch;
}

/**
 * @brief フリーカメラ専用の更新処理
 * @param dt 前フレームからの経過時間
 */
void FreeCamera::Update(float dt)
{
	// フレーム時間に応じた移動速度と回転速度の計算
	float baseMoveSpeed = BASE_MOVE_SPEED * dt;
	float rotSpeed = BASE_ROT_SPEED * dt;

	// 水平（前後左右）と垂直（上下）で独立した移動スピードを持たせる
	float moveSpeedHorizontal = baseMoveSpeed;
	float moveSpeedVertical = baseMoveSpeed;

	// Cキーで減速（上下と水平で異なる倍率をかける）
	if (GetAsyncKeyState('C') & KEY_IS_DOWN_MASK)
	{
		moveSpeedHorizontal *= SLOW_RATE_HORIZONTAL;
		moveSpeedVertical *= SLOW_RATE_VERTICAL;
	}

	// 矢印キーで回転（反転済み）
	if (GetAsyncKeyState(VK_LEFT) & KEY_IS_DOWN_MASK) m_yaw += rotSpeed;
	if (GetAsyncKeyState(VK_RIGHT) & KEY_IS_DOWN_MASK) m_yaw -= rotSpeed;
	if (GetAsyncKeyState(VK_UP) & KEY_IS_DOWN_MASK) m_pitch += rotSpeed;
	if (GetAsyncKeyState(VK_DOWN) & KEY_IS_DOWN_MASK) m_pitch -= rotSpeed;

	m_pitch = clamp_val(m_pitch, m_minPitch, m_maxPitch);

	// --- カメラの前方向と右方向を計算 ---
	DirectX::SimpleMath::Matrix rotMat = 
		DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
	DirectX::SimpleMath::Vector3 forward = 
		DirectX::SimpleMath::Vector3::TransformNormal(DirectX::SimpleMath::Vector3::Forward, rotMat);
	DirectX::SimpleMath::Vector3 right = 
		DirectX::SimpleMath::Vector3::TransformNormal(DirectX::SimpleMath::Vector3::Right, rotMat);

	// Y軸固定の平行移動ベクトル
	DirectX::SimpleMath::Vector3 flatForward = forward;
	flatForward.y = 0.0f;
	if (flatForward.LengthSquared() > EPSILON_SQUARED) flatForward.Normalize();

	// Y軸固定の平行移動ベクトル
	DirectX::SimpleMath::Vector3 flatRight = right;
	flatRight.y = 0.0f;
	if (flatRight.LengthSquared() > EPSILON_SQUARED) flatRight.Normalize();

	// I/K/J/L で移動
	if (GetAsyncKeyState('I') & KEY_IS_DOWN_MASK) m_position += flatForward * moveSpeedHorizontal;
	if (GetAsyncKeyState('K') & KEY_IS_DOWN_MASK) m_position -= flatForward * moveSpeedHorizontal;
	if (GetAsyncKeyState('L') & KEY_IS_DOWN_MASK) m_position += flatRight * moveSpeedHorizontal;
	if (GetAsyncKeyState('J') & KEY_IS_DOWN_MASK) m_position -= flatRight * moveSpeedHorizontal;

	// O/U で上下
	if (GetAsyncKeyState('O') & KEY_IS_DOWN_MASK) m_position.y += moveSpeedVertical;
	if (GetAsyncKeyState('U') & KEY_IS_DOWN_MASK) m_position.y -= moveSpeedVertical;
}

/**
 * @brief 描画用のビュー行列を取得
 * @return 現在のカメラ姿勢に基づくビュー行列
 */
DirectX::SimpleMath::Matrix FreeCamera::GetViewMatrix() const
{
	//--- カメラの前方向を計算 ---
	DirectX::SimpleMath::Matrix rotMat = 
		DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);
	DirectX::SimpleMath::Vector3 forward = 
		DirectX::SimpleMath::Vector3::TransformNormal(DirectX::SimpleMath::Vector3::Forward, rotMat);
	return DirectX::SimpleMath::Matrix::CreateLookAt(m_position, m_position + forward, DirectX::SimpleMath::Vector3::Up);
}