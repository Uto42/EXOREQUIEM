/*****************************************************************//**
 * @file    Energy.cpp
 * @brief   機体の行動リソースの残量計算および回復制御の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/GameObjects/Robot/Energy/Energy.h"

 /**
  * @brief コンストラクタ
  * @param[in] maxEnergy 最大エネルギー量
  */
Energy::Energy(float maxEnergy)
	: m_max(maxEnergy)
	, m_current(maxEnergy)
{
}

/**
 * @brief エネルギー消費
 * @param[in] amount 消費量
 * @details 消費後、残量が0未満にならないように制限する
 */
void Energy::Consume(float amount)
{
	m_current -= amount;

	if (m_current < 0.0f)
	{
		m_current = 0.0f;
	}
}

/**
 * @brief エネルギー回復
 * @param[in] amount 回復量
 * @details 回復後、残量が最大値を超えないように制限する
 */
void Energy::Recover(float amount)
{
	m_current += amount;

	if (m_current > m_max)
	{
		m_current = m_max;
	}
}

/**
 * @brief エネルギーが空か判定
 * @return true：残量なし
 */
bool Energy::IsEmpty() const
{
	return m_current <= 0.0f;
}

/**
 * @brief エネルギーが満タンか判定
 * @return true：満タン
 */
bool Energy::IsFull() const
{
	return m_current >= m_max;
}

/**
 * @brief 現在値の設定
 * @param[in] amount 設定する値
 */
void Energy::SetCurrent(float amount)
{
	m_current = amount;

	if (m_current > m_max) m_current = m_max;
	if (m_current < 0.0f) m_current = 0.0f;
}

/**
 * @brief 最大値の設定
 * @param[in] amount 設定する最大値
 */
void Energy::SetMax(float amount)
{
	m_max = amount;
	if (m_current > m_max) m_current = m_max;
}

/**
 * @brief 現在のエネルギー割合を取得
 * @return 0.0f(空) ～ 1.0f(満タン)
 */
float Energy::GetRatio() const
{
	if (m_max <= 0.0f) return 0.0f;
	return m_current / m_max;
}