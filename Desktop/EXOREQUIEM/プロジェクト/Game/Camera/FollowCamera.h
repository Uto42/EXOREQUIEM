/*****************************************************************//**
 * @file    FollowCamera.h
 * @brief   プレイヤー追従機能を提供するカメラコンポーネント
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Camera/Camera.h"

class World;

// 三人称視点（TPS）追従カメラのロジックコンポーネント
class FollowCamera : public ICameraComponent
{
public:
	// コンストラクタ
	FollowCamera(World* world);
	// デストラクタ
	virtual ~FollowCamera() = default;

	// 毎フレームの追従計算とカメラ座標の更新
	void Update(Camera& camera, float dt) override;

	// 現在のカメラ距離を取得
	float GetCurrentCameraDistance() const
	{
		return (m_overrideDistance > 0.0f) ? m_overrideDistance : DEFAULT_CAMERA_DISTANCE;
	}
	// カメラ距離の上書き設定（演出用）
	void SetDistanceOverride(float distance);

	// 注視点の上書き設定（演出用）
	void SetTargetOverride(bool enable, const DirectX::SimpleMath::Vector3& target = DirectX::SimpleMath::Vector3::Zero);

private:
	// 初回フレームのカメラ位置即時同期処理
	void InitializeFirstFrame(Camera& camera, const DirectX::SimpleMath::Vector3& targetPos, float targetHeight);

	// ターゲット（注視点）の位置を滑らかに更新
	void UpdateTargetPosition(const DirectX::SimpleMath::Vector3& targetPos, float dt, float smoothSpeed);
	// 現在のターゲットと回転から理想的なカメラ位置(Eye)を計算
	DirectX::SimpleMath::Vector3 GetDesiredEyePosition(const Camera& camera) const;
	// プレイヤー頭上の天井を考慮して動的にカメラの高さを計算する処理
	float CalculateDynamicCameraHeight() const;

	// ステージの壁との干渉を補正する処理
	DirectX::SimpleMath::Vector3 ApplyWallCollision(const Camera& camera, const DirectX::SimpleMath::Vector3& idealEye) const;
	// 地面との干渉を防ぐ処理
	void ApplyGroundCollision(Camera& camera);

private:
	// --- 定数パラメータ ---
	static constexpr float DEFAULT_CAMERA_DISTANCE = 6.5f;      //< 基本距離
	static constexpr float CAMERA_SMOOTH_SPEED = 8.0f;          //< 追従速度(Lerp係数)
	static constexpr float EVADE_CAMERA_SMOOTH_SPEED = 8.0f;    //< 回避時の追従速度
	static constexpr float HEIGHT_SMOOTH_FACTOR = 10.0f;        //< カメラ高さの補間係数
	static constexpr float MIN_CAMERA_DISTANCE = 1.2f;          //< 壁衝突時にこれ以上プレイヤーに近づけないための最低維持距離
	static constexpr float DEAD_ZONE_X = 0.05f;                 //< デッドゾーンX
	static constexpr float DEAD_ZONE_Z = 0.1f;                  //< デッドゾーンZ
	static constexpr float GROUND_LIMIT = 0.5f;                 //< 地面判定の高さリミット
	static constexpr float TARGET_OFFSET_Y = 2.5f;              //< ターゲット（注視点）の高さオフセット
	static constexpr float VERTICAL_BOOST_THRESHOLD = 2.0f;     //< 垂直追従ブーストを開始する高さの差分閾値
	static constexpr float VERTICAL_BOOST_SCALE = 25.0f;        //< 垂直追従ブーストの加速倍率
	static constexpr float MAX_INTERPOLATION_RATE = 1.0f;       //< 補間率(t)の最大上限値
	static constexpr float INITIAL_CAMERA_HEIGHT = 2.5f;        //< カメラの初期配置高さ（ローカルオフセットY）
	static constexpr float OVERRIDE_THRESHOLD = 0.0f;           //< 上書き設定が有効かを判定する境界値
	static constexpr float RESET_OVERRIDE_VALUE = -1.0f;        //< 上書き距離のリセット値
	static constexpr float EPSILON = 0.001f;                    //< ゼロ除算を防止するための極小距離閾値

	World* m_world;                                             //< ワールドコンテキストのポインタ
	float m_currentCameraHeight = 2.0f;                         //< 補間計算中の現在のカメラ高さオフセット
	DirectX::SimpleMath::Vector3 m_cameraTargetSmooth;          //< 滑らかに移動する注視点座標
	bool m_isFirstFrame = true;                                 //< 初期化時やリスポーン時のワープ判定用フラグ
	bool m_isTargetOverride = false;                            //< 注視点上書きフラグ
	DirectX::SimpleMath::Vector3 m_overrideTarget;              //< 上書き用の注視点座標
	float m_overrideDistance = RESET_OVERRIDE_VALUE;            //< 上書き用のカメラ距離
};