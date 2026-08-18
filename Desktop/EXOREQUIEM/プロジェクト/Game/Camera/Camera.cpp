/*****************************************************************//**
 * @file    Camera.cpp
 * @brief   カメラシステムの共通の器クラスの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Camera/Camera.h"
#include <algorithm>
#include <random>

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

/**
 * @brief 値を最小値と最大値の範囲内に収めるヘルパー関数
 * @param[in] v 対象の値
 * @param[in] lo 最小値
 * @param[in] hi 最大値
 * @return 範囲内にクランプされた値
 */
static constexpr float clamp_val(float v, float lo, float hi)
{
	return (v < lo) ? lo : (hi < v) ? hi : v;
}

/**
 * @brief コンストラクタ
 * @param[in] windowWidth  ウィンドウの初期横幅
 * @param[in] windowHeight ウィンドウの初期縦幅
 */
Camera::Camera(int windowWidth, int windowHeight)
	: m_eye(DEFAULT_EYE_X, DEFAULT_EYE_Y, DEFAULT_EYE_Z)
	, m_target(DEFAULT_TARGET_X, DEFAULT_TARGET_Y, DEFAULT_TARGET_Z)
	, m_up(DirectX::SimpleMath::Vector3::Up)
	, m_projection(DirectX::SimpleMath::Matrix::Identity)
	, m_xAngle(0.0f)
	, m_yAngle(0.0f)
	, m_xTmp(0.0f)
	, m_yTmp(0.0f)
	, m_mouseSensitivity(MOUSE_SENSITIVITY_RELATIVE)
	, m_minPitch(DirectX::XMConvertToRadians(PITCH_LIMIT_MIN_DEG))
	, m_maxPitch(DirectX::XMConvertToRadians(PITCH_LIMIT_MAX_DEG))
	, m_sx(1.0f)
	, m_sy(1.0f)
	, m_shakeOffset(DirectX::SimpleMath::Vector3::Zero)
{
	// ウィンドウサイズに応じた投影行列の初期化
	SetWindowSize(windowWidth, windowHeight);

	// FPS視点での操作時にカーソルが画面外に出ないよう非表示化
	ShowCursor(FALSE);
}

/**
 * @brief 毎フレームのカメラ更新処理
 * @param[in] dt 前フレームからの経過時間（秒）
 */
void Camera::Update(float dt)
{
	// マウス入力による視点角度の更新
	HandleMouseInput(DirectX::Mouse::Get().GetState());
	
	// 画面揺れオフセットの更新
	UpdateShake(dt);

	// 登録された拡張コンポーネント群（追従やロックオン等）の処理を実行
	for (auto& component : m_components)
	{
		component->Update(*this, dt);
	}
}

/**
 * @brief カメラ機能コンポーネントの追加
 * @param[in] component 追加するコンポーネントの固有ポインタ
 */
void Camera::AddComponent(std::unique_ptr<ICameraComponent> component)
{
	// コンポーネントの所有権を移動してリストに追加
	m_components.push_back(std::move(component));
}

/**
 * @brief 画面揺れ（シェイク）の開始リクエスト
 * @param[in] intensity 揺れの最大振幅強度
 * @param[in] duration  揺れが続く時間（秒）
 */
void Camera::StartShake(float intensity, float duration)
{
	// 揺れの強さと持続時間を設定
	m_shakeIntensity = intensity;
	m_shakeTimer = duration;
}

/**
 * @brief 画面揺れのタイマーおよびオフセット座標の更新
 * @param[in] dt 前フレームからの経過時間（秒）
 */
void Camera::UpdateShake(float dt)
{
	// シェイクタイマーが有効な場合のみ揺れを計算
	if (m_shakeTimer > RESET_TIMER_VALUE)
	{
		m_shakeTimer -= dt;

		// 残り時間に応じて揺れの強さを線形減衰させる
		float currentPower = m_shakeIntensity * 
			(m_shakeTimer > RESET_TIMER_VALUE ? (m_shakeTimer / SHAKE_DECAY_BASE) : RESET_TIMER_VALUE);
		if (currentPower < MIN_SHAKE_POWER) currentPower = MIN_SHAKE_POWER;

		// 各軸にランダムなシェイクオフセットを適用
		m_shakeOffset.x = dist(gen) * currentPower;
		m_shakeOffset.y = dist(gen) * currentPower;
		m_shakeOffset.z = dist(gen) * currentPower;

		// タイマー終了時にオフセットをリセット
		if (m_shakeTimer <= RESET_TIMER_VALUE)
		{
			m_shakeTimer = RESET_TIMER_VALUE;
			m_shakeOffset = DirectX::SimpleMath::Vector3::Zero;
		}
	}
	else
	{
		// 通常時は揺れをゼロに固定
		m_shakeOffset = DirectX::SimpleMath::Vector3::Zero;
	}
}

/**
 * @brief 現在のカメラ座標と注視点からビュー行列を計算・取得
 * @return 計算されたビュー行列
 */
DirectX::SimpleMath::Matrix Camera::GetViewMatrix() const
{
	// 論理座標(m_eye等)を汚染しないよう、描画用の座標計算にのみシェイクを適用する
	DirectX::SimpleMath::Vector3 shakenEye = m_eye + m_shakeOffset;
	DirectX::SimpleMath::Vector3 shakenTarget = m_target + m_shakeOffset;

	// 注視点とカメラ座標からビュー行列を計算して返す
	return DirectX::SimpleMath::Matrix::CreateLookAt(shakenEye, shakenTarget, m_up);
}

/**
 * @brief カメラの正面ベクトル（揺れなし）を取得
 * @return 正面方向の正規化されたベクトル
 */
DirectX::SimpleMath::Vector3 Camera::GetForward() const
{
	// 注視点とカメラ座標から正面方向ベクトルを算出
	DirectX::SimpleMath::Vector3 forward = m_target - m_eye;
	forward.Normalize();
	return forward;
}

/**
 * @brief カメラの右方向ベクトル（揺れなし）を取得
 * @return 右方向の正規化されたベクトル
 */
DirectX::SimpleMath::Vector3 Camera::GetRight() const
{
	// 上方向と正面方向の外積から右方向ベクトルを算出
	DirectX::SimpleMath::Vector3 forward = GetForward();
	DirectX::SimpleMath::Vector3 right = m_up.Cross(forward);
	right.Normalize();
	return right;
}

/**
 * @brief マウスの相対移動量に応じたカメラ回転角度の更新
 */
void Camera::UpdateRotationByMouse()
{
	auto mouse = DirectX::Mouse::Get().GetState();

	// 相対入力モードが有効な場合のみ回転処理を実行
	if (mouse.positionMode == DirectX::Mouse::MODE_RELATIVE)
	{
		// マウス移動量と感度から回転角度を蓄積
		float dx = static_cast<float>(mouse.x);
		float dy = static_cast<float>(mouse.y);
		m_yAngle -= dx * m_mouseSensitivity;
		m_xAngle -= dy * m_mouseSensitivity;

		// カメラの回転が真上・真下を越えて画面が反転するのを防ぐ
		m_xAngle = clamp_val(m_xAngle, m_minPitch, m_maxPitch);

		// コンポーネント参照用の一時バッファに同期
		m_yTmp = m_yAngle;
		m_xTmp = m_xAngle;
	}
}

/**
 * @brief ウィンドウ中央座標を基準としたマウス入力による回転制御
 * @param[in] mouseState マウスの入力状態クラス（未使用パラメータ）
 */
void Camera::HandleMouseInput(const DirectX::Mouse::State& mouseState)
{
	UNREFERENCED_PARAMETER(mouseState);

	// 現在のマウスカーソル座標を取得し、画面中央からの移動差分を算出
	POINT currentPos;
	GetCursorPos(&currentPos);
	m_mouseDeltaX = static_cast<float>(currentPos.x - SCREEN_CENTER_X);
	m_mouseDeltaY = static_cast<float>(currentPos.y - SCREEN_CENTER_Y);

	// 次のフレームで移動差分だけを正確に取得するため、カーソル位置を強制的に画面中央へ戻す
	SetCursorPos(SCREEN_CENTER_X, SCREEN_CENTER_Y);

	// 移動差分と感度から回転角度を更新
	m_yAngle -= m_mouseDeltaX * MOUSE_SENSITIVITY_DEFAULT;
	m_xAngle -= m_mouseDeltaY * MOUSE_SENSITIVITY_DEFAULT;
	m_xAngle = std::max(m_minPitch, std::min(m_maxPitch, m_xAngle));

	// コンポーネント参照用の一時バッファに同期
	m_yTmp = m_yAngle;
	m_xTmp = m_xAngle;
}

/**
 * @brief ウィンドウの表示解像度サイズ設定および比率の再計算
 * @param[in] windowWidth  ウィンドウの横幅
 * @param[in] windowHeight ウィンドウの縦幅
 */
void Camera::SetWindowSize(int windowWidth, int windowHeight)
{
	if (windowWidth > 0 && windowHeight > 0)
	{
		// 毎フレーム発生する除算処理の負荷を下げるため、あらかじめ逆数を保持しておく
		m_sx = 1.0f / static_cast<float>(windowWidth);
		m_sy = 1.0f / static_cast<float>(windowHeight);
	}
}

/**
 * @brief 現在設定されているウィンドウの表示解像度サイズを取得
 * @param[out] windowWidth  ウィンドウの横幅を受け取る参照
 * @param[out] windowHeight ウィンドウの縦幅を受け取る参照
 */
void Camera::GetWindowSize(int& windowWidth, int& windowHeight) const
{
	// 逆数から元のサイズを計算して返す
	windowWidth = (m_sx > 0.0f) ? static_cast<int>(1.0f / m_sx) : 0;
	windowHeight = (m_sy > 0.0f) ? static_cast<int>(1.0f / m_sy) : 0;
}

/**
 * @brief 透視投影による射影行列の設定
 * @param[in] fovRadians 画角（ラジアン）
 * @param[in] aspectRatio  アスペクト比（幅/高さ）
 * @param[in] nearZ 前方クリップ面までの距離
 * @param[in] farZ 後方クリップ面までの距離
 */
void Camera::SetProjectionMatrix(float fovRadians, float aspectRatio, float nearZ, float farZ)
{
	// 射影行列を透視投影で計算して格納
	m_projection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(fovRadians, aspectRatio, nearZ, farZ);
}

/**
 * @brief カメラの回転角度（Yaw/Pitch）の直接指定設定
 * @param[in] yawRad 左右の回転角度（ラジアン）
 * @param[in] pitchRad 上下の回転角度（ラジアン）
 */
void Camera::SetCameraYawPitch(float yawRad, float pitchRad)
{
	m_yAngle = yawRad;
	// 可動限界内に制限して格納
	m_xAngle = clamp_val(pitchRad, m_minPitch, m_maxPitch);

	// コンポーネント参照用の一時バッファに同期
	m_yTmp = m_yAngle;
	m_xTmp = m_xAngle;
}