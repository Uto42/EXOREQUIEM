/*****************************************************************//**
 * @file    Camera.h
 * @brief   カメラシステムの共通の器クラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Camera/ICameraComponent.h"

class Camera;

// 各種カメラコンポーネントを内包し、最終的な行列を管理するクラス
class Camera
{
public:
	// コンストラクタ
	Camera(int windowWidth, int windowHeight);
	// デストラクタ
	virtual ~Camera() = default;

	// 更新処理
	void Update(float dt);
	// 機能パーツの追加
	void AddComponent(std::unique_ptr<ICameraComponent> component);
	// 画面揺れ（シェイク）の開始リクエスト
	void StartShake(float intensity, float duration);
	// 旧方式のマウスドラッグ入力処理（必要に応じて使用）
	void HandleMouseInput(const DirectX::Mouse::State& mouseState);

	// カメラの現在座標を取得
	DirectX::SimpleMath::Vector3 GetEye() const { return m_eye; }
	// カメラの座標を取得するゲッター
	DirectX::SimpleMath::Vector3 GetPosition() const { return m_eye; }
	// カメラの現在座標を設定
	void SetEye(const DirectX::SimpleMath::Vector3& eye) { m_eye = eye; }

	// カメラの注視点を取得
	DirectX::SimpleMath::Vector3 GetTarget() const { return m_target; }
	// カメラの注視点を設定
	void SetTarget(const DirectX::SimpleMath::Vector3& target) { m_target = target; }

	// カメラの回転角度yを取得するゲッター
	float GetYaw() const { return m_yAngle; }
	// カメラの回転角度xを取得するゲッター
	float GetPitch() const { return m_xAngle; }
	// 左右の回転角度（一時保存）を取得
	float GetYTmp() const { return m_yTmp; }
	// カメラのY回転角度を取得
	float GetYawTmp() const { return m_yTmp; }
	// 上下の回転角度（一時保存）を取得
	float GetXTmp() const { return m_xTmp; }
	// カメラのX回転角度を取得
	float GetPitchTmp() const { return m_xTmp; }
	// カメラの回転角度を設定
	void SetCameraYawPitch(float yawRad, float pitchRad);

	// カメラの正面ベクトル（揺れなし）を取得
	DirectX::SimpleMath::Vector3 GetForward() const;
	// カメラの右方向ベクトル（揺れなし）を取得
	DirectX::SimpleMath::Vector3 GetRight() const;

	// 最終的なビュー行列を取得
	DirectX::SimpleMath::Matrix GetViewMatrix() const;
	// 射影行列を取得
	DirectX::SimpleMath::Matrix GetProjectionMatrix() const { return m_projection; }
	// 射影行列を設定
	void SetProjectionMatrix(float fovRadians, float aspectRatio, float nearZ, float farZ);

	// ウィンドウサイズを取得
	void GetWindowSize(int& windowWidth, int& windowHeight) const;
	// ウィンドウサイズを設定
	void SetWindowSize(int windowWidth, int windowHeight);
	// マウスの移動量Xを取得するゲッター
	float GetMouseDeltaX() const { return m_mouseDeltaX; }
	// マウスの移動量Yを取得するゲッター
	float GetMouseDeltaY() const { return m_mouseDeltaY; }

private:
	// マウス移動による回転更新
	void UpdateRotationByMouse();
	// 画面揺れのタイマー更新
	void UpdateShake(float dt);

private:
	static constexpr float DEFAULT_EYE_X = 0.0f;                //< 初期カメラ座標X
	static constexpr float DEFAULT_EYE_Y = 2.0f;                //< 初期カメラ座標Y
	static constexpr float DEFAULT_EYE_Z = -5.0f;               //< 初期カメラ座標Z
	static constexpr float DEFAULT_TARGET_X = 0.0f;             //< 初期注視点座標X
	static constexpr float DEFAULT_TARGET_Y = 1.0f;             //< 初期注視点座標Y
	static constexpr float DEFAULT_TARGET_Z = 0.0f;             //< 初期注視点座標Z
	static constexpr float PITCH_LIMIT_MIN_DEG = -60.0f;        //< 見下ろし限界角度（度数法）
	static constexpr float PITCH_LIMIT_MAX_DEG = 85.0f;         //< 見上げ限界角度（度数法）
	static constexpr float MOUSE_SENSITIVITY_RELATIVE = 0.0001f; //< 相対入力モード時のマウス感度
	static constexpr float SHAKE_DECAY_BASE = 0.5f;             //< 画面揺れの減衰計算に用いる基準時間
	static constexpr float MIN_SHAKE_POWER = 0.0f;              //< 画面揺れ強度の最小下限値
	static constexpr float RESET_TIMER_VALUE = 0.0f;            //< タイマーリセット時の設定値
	static constexpr float ZOOM_SCALING_FACTOR = 100.0f;        //< マウスホイール累積値からズーム量への変換倍率
	static constexpr float MOUSE_SENSITIVITY_DEFAULT = 0.002f;  //< 中央固定方式でのマウス感度基準値
	static constexpr long  SCREEN_CENTER_X = 1920 / 2;          //< カーソルを固定するウィンドウ中央座標X
	static constexpr long  SCREEN_CENTER_Y = 1080 / 2;          //< カーソルを固定するウィンドウ中央座標Y

	// --- カメラの基本姿勢 ---
	DirectX::SimpleMath::Vector3 m_eye;                //< カメラ座標
	DirectX::SimpleMath::Vector3 m_target;             //< 注視点座標
	DirectX::SimpleMath::Vector3 m_up;                 //< 上方向ベクトル
	DirectX::SimpleMath::Matrix m_projection;          //< 射影行列

	// --- 角度・回転情報 ---
	float m_xAngle;                                    //< Pitch (上下回転角)
	float m_yAngle;                                    //< Yaw (左右回転角)
	float m_xTmp;                                      //< Pitch一時保存
	float m_yTmp;                                      //< Yaw一時保存
	float m_minPitch;                                  //< 上方回転限界のラジアン値
	float m_maxPitch;                                  //< 下方回転限界のラジアン値

	// --- 画面・入力状態 ---
	float m_mouseSensitivity;                          //< マウス操作の感度係数
	float m_sx;                                        //< 画面横幅の逆数
	float m_sy;                                        //< 画面縦幅の逆数
	float m_mouseDeltaX = 0.0f;                        //< 今フレームのマウス移動量X
	float m_mouseDeltaY = 0.0f;                        //< 今フレームのマウス移動量Y

	// --- 特殊効果（画面揺れ） ---
	float m_shakeIntensity = 0.0f;                     //< 画面揺れの最大振幅
	float m_shakeTimer = 0.0f;                         //< 画面揺れの残り時間秒数
	DirectX::SimpleMath::Vector3 m_shakeOffset;        //< 実際の揺れオフセット座標

	// --- 拡張機能 ---
	std::vector<std::unique_ptr<ICameraComponent>> m_components; //< 登録されたコンポーネントのリスト
};