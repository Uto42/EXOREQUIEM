/*****************************************************************//**
 * @file    Particle.h
 * @brief   個別のパーティクル粒子の物理挙動およびパラメータ補間処理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

/**
 * @class Particle
 * @brief 1つのパーティクル粒子の状態（座標、速度、色、スケール）を管理する
 */
class Particle
{
public:
    // コンストラクタ
    Particle(float life, const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& velocity,
        const DirectX::SimpleMath::Vector3& accele, const DirectX::SimpleMath::Vector3& startScale,
        const DirectX::SimpleMath::Vector3& endScale, const DirectX::SimpleMath::Color& startColor,
        const DirectX::SimpleMath::Color& endColor);
    // デストラクタ
    ~Particle();
    // 更新処理
    bool Update(float dt);

	// 現在の座標を取得する
    const DirectX::SimpleMath::Vector3& GetPosition() const { return m_position; }
	// 現在のスケールを取得する
    const DirectX::SimpleMath::Vector3& GetNowScale() const { return m_nowScale; }
	// 現在の色を取得する
    const DirectX::SimpleMath::Color& GetNowColor() const { return m_nowColor; }

private:
    // 物理挙動パラメータ
    DirectX::SimpleMath::Vector3 m_position;    //< 現在の座標
    DirectX::SimpleMath::Vector3 m_velocity;    //< 現在の速度
    DirectX::SimpleMath::Vector3 m_accele;      //< 加速度

    // 形状・演出パラメータ
    DirectX::SimpleMath::Vector3 m_nowScale;    //< 現在のスケール
    DirectX::SimpleMath::Vector3 m_startScale;  //< 開始時のスケール
    DirectX::SimpleMath::Vector3 m_endScale;    //< 終了時のスケール

    DirectX::SimpleMath::Color   m_nowColor;    //< 現在の色
    DirectX::SimpleMath::Color   m_startColor;  //< 開始時の色
    DirectX::SimpleMath::Color   m_endColor;    //< 終了時の色

    // 寿命管理
    float m_life;                               //< 残り寿命
    float m_startLife;                          //< 開始時の最大寿命
};