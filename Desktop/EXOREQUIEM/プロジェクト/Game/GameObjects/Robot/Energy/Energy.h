/*****************************************************************//**
 * @file    Energy.h
 * @brief   機体の行動リソース（ブースト・特殊アクション）の管理クラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

class Energy
{
public:
	// --- 調整用定数パラメータ ---
	static constexpr float DEFAULT_MAX_ENERGY = 100.0f; //< 機体のデフォルト最大エネルギー量

	// コンストラクタ
	Energy(float maxEnergy = DEFAULT_MAX_ENERGY);
	// デストラクタ
	~Energy() = default;

	// エネルギーを消費する
	void Consume(float amount);

	// エネルギーを回復する
	void Recover(float amount);

	// エネルギーが空か判定
	bool IsEmpty() const;
	// エネルギーが満タンか判定
	bool IsFull() const;

	// 現在値を設定
	void SetCurrent(float amount);
	// 現在値を取得
	float GetCurrent() const { return m_current; }

	// 最大値を設定
	void SetMax(float amount);
	// 最大値を取得
	float GetMax() const { return m_max; }

	// 現在の割合を取得（0.0～1.0）
	float GetRatio() const;

private:
	float m_current; //< 現在のエネルギー量
	float m_max;     //< 最大エネルギー量
};