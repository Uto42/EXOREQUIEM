/*****************************************************************//**
 * @file    UserResources.h
 * @brief   入力デバイスや共有リソースへのアクセスを仲介する管理クラス
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "pch.h"
#include "Game/Common/StepTimer.h"
#include "Game/Common/DeviceResources.h"
#include "Game/Common/DebugFont.h"

class UserResources
{
private:
	DX::StepTimer* m_timer;										 //< ステップタイマー
	DX::DeviceResources* m_deviceResources;						 //< デバイスリソース
	DirectX::Keyboard::KeyboardStateTracker* m_keyboardTracker;  //< キーボードトラッカー
	DirectX::Mouse::ButtonStateTracker* m_mouseTracker;		     //< マウストラッカー
	DebugFont* m_debugFont;										 //< デバッグフォント
	DirectX::CommonStates* m_states;							 //< 共通ステート

public:
	// コンストラクタ
	UserResources()
		: m_timer(nullptr)
		, m_deviceResources(nullptr)
		, m_keyboardTracker(nullptr)
		, m_mouseTracker(nullptr)
		, m_debugFont(nullptr)
		, m_states(nullptr)
	{
	}

	// ステップタイマー
	void SetStepTimerStates(DX::StepTimer* timer) { m_timer = timer; }
	DX::StepTimer* GetStepTimer() { return m_timer; }

	// デバイスリソース
	void SetDeviceResources(DX::DeviceResources* deviceResources) { m_deviceResources = deviceResources; }
	DX::DeviceResources* GetDeviceResources() { return m_deviceResources; }

	// キーボードトラッカー
	void SetKeyboardStateTracker(DirectX::Keyboard::KeyboardStateTracker* tracker) { m_keyboardTracker = tracker; }
	DirectX::Keyboard::KeyboardStateTracker* GetKeyboardStateTracker() { return m_keyboardTracker; }

	// マウストラッカー
	void SetMouseStateTracker(DirectX::Mouse::ButtonStateTracker* tracker) { m_mouseTracker = tracker; }
	DirectX::Mouse::ButtonStateTracker* GetMouseStateTracker() { return m_mouseTracker; }

	// デバッグフォント
	void SetDebugFont(DebugFont* debugFont) { m_debugFont = debugFont; }
	DebugFont* GetDebugFont() { return m_debugFont; }

	// 共通ステート
	void SetCommonStates(DirectX::CommonStates* states) { m_states = states; }
	DirectX::CommonStates* GetCommonStates() { return m_states; }
};