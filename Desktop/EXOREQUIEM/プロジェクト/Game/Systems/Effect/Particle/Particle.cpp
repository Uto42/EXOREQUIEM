/*****************************************************************//**
 * @file    Particle.cpp
 * @brief   個別のパーティクル粒子の物理挙動およびパラメータ補間処理の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Effect/Particle/Particle.h"

/**
 * @brief コンストラクタ
 * @param[in] life 寿命(秒)
 * @param[in] pos 初期座標
 * @param[in] velocity 初期速度
 * @param[in] accele 加速度
 * @param[in] startScale 開始スケール
 * @param[in] endScale 終了スケール
 * @param[in] startColor 開始色
 * @param[in] endColor 終了色
 */
Particle::Particle(float life, const DirectX::SimpleMath::Vector3& pos,
    const DirectX::SimpleMath::Vector3& velocity, const DirectX::SimpleMath::Vector3& accele,
    const DirectX::SimpleMath::Vector3& startScale, const DirectX::SimpleMath::Vector3& endScale, 
    const DirectX::SimpleMath::Color& startColor, const DirectX::SimpleMath::Color& endColor)
    : m_life(life)
    , m_startLife(life)
    , m_position(pos)
    , m_velocity(velocity)
    , m_accele(accele)
    , m_nowScale(startScale)
    , m_startScale(startScale)
    , m_endScale(endScale)
    , m_nowColor(startColor)
    , m_startColor(startColor)
    , m_endColor(endColor)
{
}

/**
 * @brief デストラクタ
 */
Particle::~Particle()
{
}

/**
 * @brief 更新処理
 * @details 寿命のカウント、速度・座標の更新、寿命に基づいたサイズと色の線形補間を行う
 * @param[in] dt デルタタイム
 * @return bool 存続しているなら true, 寿命が尽きたら false
 */
bool Particle::Update(float dt)
{
    // 寿命を減らす
    m_life -= dt;

    // 寿命が尽きたら false を返す
    if (m_life <= 0.0f)
    {
        return false;
    }

    // 進行割合 (0.0 -> 1.0)
    // 寿命の減少に合わせて 0 から 1 へ変化させる
    float ratio = 1.0f - (m_life / m_startLife);

    // 物理挙動の更新
    m_velocity += m_accele * dt;
    m_position += m_velocity * dt;

    // スケールと色の線形補間 (Lerp)
    m_nowScale = DirectX::SimpleMath::Vector3::Lerp(m_startScale, m_endScale, ratio);
    m_nowColor = DirectX::SimpleMath::Color::Lerp(m_startColor, m_endColor, ratio);

    return true;
}