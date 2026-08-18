/*****************************************************************//**
 * @file    PauseManager.h
 * @brief   ゲームの一時停止（ポーズ）状態の監視およびポーズ画面のオーバーレイ描画
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include "Game/UI/Menu/MenuUIManager.h"

class PauseManager
{
public:
	// メニューの種類
	enum class MenuType { Resume, Retry, Title, Quit, Max, None };

	// コンストラクタ
	PauseManager();
	// デストラクタ
	~PauseManager() = default;
	// 初期化処理
	void Initialize(ID3D11Device* device);
	// 更新処理
	bool Update(const DirectX::Keyboard::State& kb, float dt);
	// 描画処理
	void Render(DirectX::SpriteBatch* spriteBatch, const RECT& outputSize);

	// ポーズ状態の取得
	bool IsPaused() const { return m_isPaused; }

	// ポーズ状態の設定
	void SetPaused(bool active) { m_isPaused = active; }

	// 決定されたメニューの取得
	MenuType GetExecutedMenu() const { return m_executedMenu; }

	// 決定されたメニューのクリア
	void ClearExecutedMenu() { m_executedMenu = MenuType::None; }

private:
	// メニューのレイアウト再計算
	void UpdateMenuLayout(float W, float H);

private:
	// --- 定数パラメータ ---
	static constexpr uint32_t WHITE_PIXEL_VALUE = 0xFFFFFFFF;            //< ダミーテクスチャ用の白色ピクセルデータ
	static constexpr float BASE_RESOLUTION_H = 1080.0f;                  //< UI基準縦解像度
	static constexpr float BACKGROUND_ALPHA = 0.5f;                      //< 背景の暗転アルファ値
	static constexpr float HALF_RATIO = 0.5f;                            //< 中心や半分を計算するための割合
	static constexpr float SELECTED_EFFECT_SCALE = 1.1f;                 //< 選択中の拡大倍率
	static constexpr float MENU_BASE_X_RATIO = 0.20f;                    //< メニュー配置X開始位置
	static constexpr float MENU_BASE_Y_RATIO = 0.30f;                    //< メニュー配置Y開始位置
	static constexpr float MENU_LINE_SPACING = 140.0f;                   //< メニュー項目間の縦の隙間
	static constexpr float BLINK_SPEED = 5.0f;                           //< ロゴの点滅速度
	static constexpr float MENU_FRAME_SCALE_X = 0.55f;                   //< メニュー選択枠の横スケール
	static constexpr float MENU_FRAME_SCALE_Y = 0.30f;                   //< メニュー選択枠の縦スケール
	static constexpr float GUIDE_POS_X_RATIO = 0.70f;                    //< 操作説明のX配置位置
	static constexpr float GUIDE_POS_Y_RATIO = 0.55f;                    //< 操作説明のY配置位置
	static constexpr float GUIDE_TARGET_HEIGHT_RATIO = 0.58f;            //< 操作説明の表示高さ比率
	static constexpr float LOGO_POS_Y_RATIO = 0.12f;                     //< ポーズロゴのY配置位置
	static constexpr float TEXT_SCALE_MULTIPLIER = 0.7f;                 //< 枠に対する文字の相対スケール
	static constexpr float TEXT_COLOR_NORMAL = 1.0f;                     //< 通常時の文字色
	static constexpr int CURSOR_VISIBLE_THRESHOLD = 0;                   //< カーソル表示の判定しきい値

	//--- メンバ変数 ---
	bool m_isPaused;                                                     //< ポーズ状態フラグ
	MenuType m_executedMenu;                                             //< 決定されたメニュー
	float m_timer;                                                       //< 演出用タイマー
	float m_guiScale;                                                    //< GUIスケール

	//--- 入力トラッカー ---
	DirectX::Mouse::ButtonStateTracker m_mouseTracker;                    //< マウストラッカー
	DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;            //< キーボードトラッカー
	MenuUIManager m_menuUI;                                               //< メニューUIマネージャーコンポーネント

	// --- テクスチャ ---
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_blackTexture;     //< 暗転用テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pauseLogoTexture; //< ロゴテクスチャ
	DirectX::SimpleMath::Vector2 m_sizePauseLogo;                        //< ロゴサイズ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_frameTexture;     //< 選択枠テクスチャ
	DirectX::SimpleMath::Vector2 m_frameSize;                            //< 選択枠サイズ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_howToPlayTexture; //< 操作説明テクスチャ
	DirectX::SimpleMath::Vector2 m_sizeHowToPlayImage;                    //< 操作説明サイズ
};