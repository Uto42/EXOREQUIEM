/*****************************************************************//**
 * @file    WeaponRuntime.h
 * @brief   すべての武装（マシンガン、ミサイル等）の共通タイマーおよび残弾管理を行うコンポーネントクラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <algorithm>

class WeaponRuntime
{
public:
	// 既定コンストラクタ
	WeaponRuntime()
		: m_maxAmmo(0)
		, m_currentAmmo(0)
		, m_fireInterval(0.0f)
		, m_fireTimer(0.0f)
		, m_reloadTime(0.0f)
		, m_reloadCoolDownTimer(0.0f)
		, m_isReloading(false)
	{
	}

	// パラメータ指定コンストラクタ
	WeaponRuntime(int maxAmmo, float fireInterval, float reloadTime)
		: m_maxAmmo(maxAmmo)
		, m_currentAmmo(maxAmmo)
		, m_fireInterval(fireInterval)
		, m_fireTimer(0.0f)
		, m_reloadTime(reloadTime)
		, m_reloadCoolDownTimer(0.0f)
		, m_isReloading(false)
	{
	}

	// デストラクタ
	~WeaponRuntime() = default;

	// タイマーとリロード状態の更新処理
	void Update(float dt)
	{
		if (m_fireTimer > 0.0f) m_fireTimer -= dt;

		if (m_isReloading)
		{
			m_reloadCoolDownTimer += dt;
			m_currentAmmo = 0;

			if (m_reloadCoolDownTimer >= m_reloadTime)
			{
				m_currentAmmo = m_maxAmmo;
				m_isReloading = false;
				m_reloadCoolDownTimer = 0.0f;
			}
		}
		else if (m_currentAmmo <= 0)
		{
			StartReload();
		}
	}

	// 手動リロードの開始
	void StartReload()
	{
		if (m_currentAmmo < m_maxAmmo && !m_isReloading)
		{
			m_isReloading = true;
			m_reloadCoolDownTimer = 0.0f;
		}
	}

	// 現在の残弾数を取得
	int GetCurrentAmmo() const { return m_currentAmmo; }
	// 最大装弾数を取得
	int GetMaxAmmo() const { return m_maxAmmo; }
	// リロード中かどうかを取得
	bool IsReloading() const { return m_isReloading; }
	// リロードの進行率を0.0f～1.0fで取得
	float GetReloadTimeRate() const
	{
		if (m_isReloading)
		{
			return std::min(m_reloadCoolDownTimer / m_reloadTime, 1.0f);
		}
		return 1.0f;
	}

	// 連射タイマーの取得
	float GetFireTimer() const { return m_fireTimer; }
	// 連射タイマーの設定
	void SetFireTimer(float time) { m_fireTimer = time; }
	// 連射間隔の取得
	float GetFireInterval() const { return m_fireInterval; }
	// 残弾数のデクリメント
	void DecrementAmmo() { if (m_currentAmmo > 0) m_currentAmmo--; }

private:
	// --- 状態値・パラメータ ---
	int m_maxAmmo;                       //< 最大装弾数
	int m_currentAmmo;                   //< 現在の残弾数
	float m_fireInterval;                //< 連射間隔（秒）
	float m_fireTimer;                   //< 連射タイマー
	float m_reloadTime;                  //< リロードにかかる総時間（秒）
	float m_reloadCoolDownTimer;         //< リロード経過タイマー
	bool m_isReloading;                  //< リロード中かどうかのフラグ
};