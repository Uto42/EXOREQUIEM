/*****************************************************************//**
 * @file    LockOnCamera.h
 * @brief   ロックオン時のエイムアシストおよびプレイヤーへの向き反映を行うカメラコンポーネント
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Camera/Camera.h"

class World;
class LockOnSystem;
class Robot;

/**
 * @class LockOnCamera
 * @brief ハードロックオン時の視点自動追従を提供するカメラコンポーネント
 */
class LockOnCamera : public ICameraComponent
{
public:
	// コンストラクタ
	LockOnCamera(World* world, LockOnSystem* lockOnSystem);
	// デストラクタ
	virtual ~LockOnCamera() = default;

	// 更新処理
	void Update(Camera& camera, float dt) override;

private:
	// ロックオン時の視点アシスト処理
	void ApplyLockOnAssist(Camera& camera, float dt);
	// マウス入力によるアシスト率の更新
	void UpdateAssistRate(float mouseDeltaX, float mouseDeltaY, float dt);
	// ターゲットとの位置関係から理想のカメラ角度を計算
	void CalculateIdealAngles(Camera& camera, Robot* target, float& outYaw, float& outPitch) const;
	// 現在の角度から理想の角度へカメラを追従させる
	void ApplyTracking(Camera& camera, float idealYaw, float idealPitch, float dt) const;
	// カメラの向き情報をプレイヤーに反映
	void ReflectToPlayer(Camera& camera);

private:
	// --- 定数パラメータ ---
	static constexpr float EPSILON_DIST = 0.001f;           //< ゼロ除算を防止するための極小距離閾値
	static constexpr float SWING_RELEASE_SPEED = 0.35f;     //< アシストを強制解除するスイング速度
	static constexpr float MOUSE_INPUT_MIN = 0.05f;         //< アシスト減衰を開始する入力下限
	static constexpr float MOUSE_INPUT_MAX = 0.50f;         //< アシストが完全に切れる入力上限
	static constexpr float ASSIST_SMOOTH_RETURN = 8.0f;     //< アシストが強まる方向の補間速度
	static constexpr float ASSIST_SMOOTH_RELEASE = 30.0f;   //< アシストが弱まる方向の補間速度
	static constexpr float ASSIST_MIN_THRESHOLD = 0.001f;   //< 処理をスキップするアシスト率の下限
	static constexpr float TARGET_OFFSET_Y = 12.5f;         //< 敵の胸の高さを狙うオフセット
	static constexpr float PITCH_FLAT_START_DIST = 40.0f;   //< 視線を水平に戻し始める距離
	static constexpr float PITCH_FLAT_MIN_DIST = 10.0f;     //< 完全に水平になる距離
	static constexpr float TRACK_RATE = 15.0f;              //< カメラが敵を追従する基本スピード
	static constexpr float INNER_YAW = 0.05f;               //< 左右のデッドゾーン
	static constexpr float OUTER_YAW = 0.25f;               //< 左右の最大追従開始ライン
	static constexpr float INNER_PITCH = 0.03f;             //< 上下のデッドゾーン
	static constexpr float OUTER_PITCH = 0.16f;             //< 上下の最大追従開始ライン

	// --- メンバ変数 ---
	World* m_world;                //< ワールドコンテキストのポインタ
	LockOnSystem* m_lockOnSystem;  //< ロックオンシステムのポインタ
	float m_assistRate = 0.0f;     //< アシスト率の平滑化用
};