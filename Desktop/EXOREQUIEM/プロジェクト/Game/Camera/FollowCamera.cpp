/*****************************************************************//**
 * @file    FollowCamera.cpp
 * @brief   プレイヤー追従機能を提供するカメラコンポーネントの実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Camera/FollowCamera.h"
#include "Game/World/World.h"
#include "Game/GameObjects/Robot/State/RobotStateTypes.h"
#include "Game/GameObjects/Player/Player.h"
#include "Game/Stage/StageManager.h"

/**
 * @brief コンストラクタ
 * @param[in] world ワールドコンテキストへのポインタ
 */
FollowCamera::FollowCamera(World* world)
	: m_world(world)
	, m_cameraTargetSmooth(DirectX::SimpleMath::Vector3::Zero)
{
}

/**
 * @brief 毎フレームの追従計算とカメラ座標の更新
 * @param[in,out] camera カメラ本体の参照
 * @param[in] dt 前フレームからの経過時間（秒）
 */
void FollowCamera::Update(Camera& camera, float dt)
{
	if (!m_world) return;
	// プレイヤーとワールド情報の取得
	Player* player = m_world->GetPlayer();
	if (!player) return;

	// プレイヤーの状態を考慮して最終的なターゲット座標を決定
	DirectX::SimpleMath::Vector3 playerPos = player->GetPosition();
	bool isPlayerEvading = (player->GetCurrentStateType() == RobotStateTypes::Evade);
	DirectX::SimpleMath::Vector3 actualTargetPos = m_isTargetOverride ? m_overrideTarget : playerPos;

	// カメラの理想的な高さを取得
	float targetHeight = CalculateDynamicCameraHeight();

	// 初回フレームの即時同期（ワープやリスポーン時のカメラ飛び防止）
	if (m_isFirstFrame)
	{
		InitializeFirstFrame(camera, actualTargetPos, targetHeight);
		return;
	}

	// ターゲットの高さと水平座標を滑らかに補間して追従
	m_currentCameraHeight += (targetHeight - m_currentCameraHeight) * HEIGHT_SMOOTH_FACTOR * dt;
	float currentSmoothSpeed = isPlayerEvading ? EVADE_CAMERA_SMOOTH_SPEED : CAMERA_SMOOTH_SPEED;
	UpdateTargetPosition(actualTargetPos, dt, currentSmoothSpeed);
	camera.SetTarget(m_cameraTargetSmooth);

	// 理想のカメラ座標を算出し、壁・地面との干渉を補正
	DirectX::SimpleMath::Vector3 idealEye = GetDesiredEyePosition(camera);
	idealEye = ApplyWallCollision(camera, idealEye);

	// 最終的なカメラ座標を設定
	camera.SetEye(idealEye);
	// 地面との干渉を防ぐための補正
	ApplyGroundCollision(camera);
}

/**
 * @brief 初回フレームのカメラ位置即時同期処理
 * @param[in,out] camera カメラ本体の参照
 * @param[in] targetPos 基準となる追従ターゲット座標
 * @param[in] targetHeight 目標とするカメラの高さ
 */
void FollowCamera::InitializeFirstFrame(Camera& camera, const DirectX::SimpleMath::Vector3& targetPos, float targetHeight)
{
	m_currentCameraHeight = targetHeight;
	m_cameraTargetSmooth = targetPos + DirectX::SimpleMath::Vector3(0.0f, TARGET_OFFSET_Y, 0.0f);

	// ターゲット座標を即時反映してカメラの注視点を設定
	camera.SetTarget(m_cameraTargetSmooth);
	camera.SetEye(GetDesiredEyePosition(camera));

	// 地面との干渉を防ぐための補正
	ApplyGroundCollision(camera);

	m_isFirstFrame = false;
}

/**
 * @brief ターゲット（注視点）の位置をデッドゾーンおよび垂直ブーストを考慮して滑らかに更新
 * @param[in] targetPos 追従対象の基準座標
 * @param[in] dt 前フレームからの経過時間（秒）
 * @param[in] smoothSpeed 補間追従速度
 */
void FollowCamera::UpdateTargetPosition(const DirectX::SimpleMath::Vector3& targetPos, float dt, float smoothSpeed)
{
	// 理想的な注視点の算出
	DirectX::SimpleMath::Vector3 idealLookAt = targetPos + DirectX::SimpleMath::Vector3(0.0f, TARGET_OFFSET_Y, 0.0f);

	float nextX = m_cameraTargetSmooth.x;
	float nextZ = m_cameraTargetSmooth.z;

	// デッドゾーン外であればX軸とZ軸を個別に滑らかに補間
	if (fabs(idealLookAt.x - m_cameraTargetSmooth.x) > DEAD_ZONE_X)
	{
		nextX = m_cameraTargetSmooth.x + (idealLookAt.x - m_cameraTargetSmooth.x) * smoothSpeed * dt;
	}
	if (fabs(idealLookAt.z - m_cameraTargetSmooth.z) > DEAD_ZONE_Z)
	{
		nextZ = m_cameraTargetSmooth.z + (idealLookAt.z - m_cameraTargetSmooth.z) * smoothSpeed * dt;
	}

	// Y軸（垂直方向）の補間計算
	float diffY = idealLookAt.y - m_cameraTargetSmooth.y;
	float speedY = smoothSpeed;

	// 大ジャンプや急落下時にカメラが取り残されないよう垂直追従をブーストする
	if (diffY > VERTICAL_BOOST_THRESHOLD)
	{
		float excess = diffY - VERTICAL_BOOST_THRESHOLD;
		speedY += excess * VERTICAL_BOOST_SCALE;
	}

	float t_Vertical = speedY * dt;
	if (t_Vertical > MAX_INTERPOLATION_RATE) t_Vertical = MAX_INTERPOLATION_RATE;

	// 最終的な注視点座標の確定
	float nextY = m_cameraTargetSmooth.y + diffY * t_Vertical;
	m_cameraTargetSmooth = DirectX::SimpleMath::Vector3(nextX, nextY, nextZ);
}

/**
 * @brief 現在のターゲット座標とカメラの回転角度から理想的なカメラ位置(Eye)を計算
 * @param[in] camera カメラ本体の参照
 * @return 計算された理想的なカメラの座標
 */
DirectX::SimpleMath::Vector3 FollowCamera::GetDesiredEyePosition(const Camera& camera) const
{
	// 演出等の上書き設定を考慮してカメラの基本距離を決定
	float dist = (m_overrideDistance > OVERRIDE_THRESHOLD) ? m_overrideDistance : DEFAULT_CAMERA_DISTANCE;
	DirectX::SimpleMath::Vector3 baseOffset(0.0f, m_currentCameraHeight, -dist);

	// カメラの回転角度（Yaw/Pitch）を行列化してオフセットに適用
	DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateRotationX(-camera.GetXTmp()) * 
		DirectX::SimpleMath::Matrix::CreateRotationY(camera.GetYTmp());

	DirectX::SimpleMath::Vector3 offset = DirectX::SimpleMath::Vector3::Transform(baseOffset, rot);

	return m_cameraTargetSmooth + offset;
}

/**
 * @brief ステージの壁との干渉を補正する処理
 * @param[in] camera カメラ本体の参照
 * @param[in] idealEye 壁判定前の理想のカメラ座標
 * @return 壁との干渉を解決した後のカメラ座標
 */
DirectX::SimpleMath::Vector3 FollowCamera::ApplyWallCollision(const Camera& camera, 
	const DirectX::SimpleMath::Vector3& idealEye) const
{
	StageManager* stageManager = m_world->GetStageManager();
	if (!stageManager) return idealEye;

	DirectX::SimpleMath::Vector3 adjustedEye = stageManager->AdjustCameraPosition(m_cameraTargetSmooth, idealEye);

	// 壁補正でプレイヤーに近づきすぎて視界が塞がるのを防ぐ
	DirectX::SimpleMath::Vector3 toCamera = adjustedEye - m_cameraTargetSmooth;
	float currentDist = toCamera.Length();

	if (currentDist < MIN_CAMERA_DISTANCE)
	{
		if (currentDist > EPSILON)
		{
			toCamera.Normalize();
			adjustedEye = m_cameraTargetSmooth + toCamera * MIN_CAMERA_DISTANCE;
		}
		else
		{
			// 完全に重なった場合のゼロ除算を防ぐため、強制的にカメラの背後へ退避する
			DirectX::SimpleMath::Vector3 fallbackDir = -camera.GetForward();
			if (fallbackDir.LengthSquared() < EPSILON) fallbackDir = DirectX::SimpleMath::Vector3::Backward;
			adjustedEye = m_cameraTargetSmooth + fallbackDir * MIN_CAMERA_DISTANCE;
		}
	}

	return adjustedEye;
}

/**
 * @brief カメラの最低高度リミット制限による地面との衝突干渉防止処理
 * @param[in,out] camera カメラ本体の参照
 */
void FollowCamera::ApplyGroundCollision(Camera& camera)
{
	DirectX::SimpleMath::Vector3 currentEye = camera.GetEye();
	DirectX::SimpleMath::Vector3 target = camera.GetTarget();

	// カメラが地面の最低高度を下回った場合の押し出し補正
	if (currentEye.y < GROUND_LIMIT)
	{
		DirectX::SimpleMath::Vector3 dir = target - currentEye;

		// 視線角度を維持したまま地面との干渉を解消する
		if (dir.y > EPSILON)
		{
			float t = (GROUND_LIMIT - currentEye.y) / dir.y;
			currentEye = currentEye + dir * t;
		}
		else
		{
			currentEye.y = GROUND_LIMIT;
		}

		camera.SetEye(currentEye);
	}
}

/**
 * @brief プレイヤー頭上の空間をチェックし、適切なカメラの高さオフセットを返す
 * @return 計算されたカメラの高さ(Y軸オフセット)
 */
float FollowCamera::CalculateDynamicCameraHeight() const
{
	return INITIAL_CAMERA_HEIGHT;
}

/**
 * @brief 演出等のための注視点座標の上書き設定
 * @param[in] enable trueで上書き有効、falseで通常追従
 * @param[in] target 上書きする対象の座標
 */
void FollowCamera::SetTargetOverride(bool enable, const DirectX::SimpleMath::Vector3& target)
{
	m_isTargetOverride = enable;
	m_overrideTarget = target;
}

/**
 * @brief 演出等のためのカメラ距離の上書き設定
 * @param[in] distance 上書きするカメラ距離（負の値で上書き解除）
 */
void FollowCamera::SetDistanceOverride(float distance)
{
	m_overrideDistance = distance;
}