/*****************************************************************//**
 * @file    PlayerStatusUI.h
 * @brief   プレイヤーのHP・エネルギー残量のゲージ表示
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

class Player;

/**
 * @class PlayerStatusUI
 * @brief プレイヤーのステータス（HP/Energy）を画面上に描画するクラス
 */
class PlayerStatusUI
{
public:
	// 基準点の種類
	enum class Anchor {
		TopLeft,              //< 左上
		TopRight,             //< 右上
		BottomCenter,         //< 下中央
	};

	// コンストラクタ
	PlayerStatusUI();
	// デストラクタ
	~PlayerStatusUI();
	// 初期化処理
	void Initialize(ID3D11Device* device);
	// 描画処理
	void Render(DirectX::SpriteBatch* spriteBatch, const Player* player, const D3D11_VIEWPORT& viewport);

private:
	// 座標計算処理
	DirectX::SimpleMath::Vector2 CalculateDrawPos(
		const D3D11_VIEWPORT& viewport,
		Anchor anchor,
		const DirectX::SimpleMath::Vector2& size,
		const DirectX::SimpleMath::Vector2& offset
	);

	// プレイヤーのHPバーを描画
	void DrawHPBar(
		DirectX::SpriteBatch* spriteBatch,
		const Player* player,
		const D3D11_VIEWPORT& viewport,
		float guiScale);
	// プレイヤーのエネルギーバーを描画
	void DrawEnergyBar(DirectX::SpriteBatch* spriteBatch,
		const Player* player, 
		const D3D11_VIEWPORT& viewport,
		float guiScale);
	// 共通のゲージ描画処理
	void DrawGauge(DirectX::SpriteBatch* spriteBatch, 
		ID3D11ShaderResourceView* barTexture,
		const DirectX::SimpleMath::Vector2& pos,
		const DirectX::SimpleMath::Vector2& size,
		float ratio);

private:
	// --- スケール・画面基準の定数 ---
    static constexpr float HALF_RATIO = 0.5f;						//< 半分を求める係数
    static constexpr float BASE_SCREEN_HEIGHT = 720.0f;				//< 基準とする画面解像度（縦）

    // --- 各ゲージの配置・基本サイズ定数 ---
    static constexpr float HP_BAR_BASE_WIDTH = 350.0f;				//< HPゲージの基本横幅(px)
    static constexpr float HP_BAR_BASE_HEIGHT = 25.0f;				//< HPゲージの基本縦幅(px)
    static constexpr float HP_OFFSET_X_RATIO = 0.04f;				//< 画面横幅に対する配置オフセット比率
    static constexpr float HP_OFFSET_Y_RATIO = 0.85f;				//< 画面縦幅に対する配置オフセット比率

    static constexpr float ENG_BAR_BASE_WIDTH = 300.0f;				//< エネルギーゲージの基本横幅(px)
    static constexpr float ENG_BAR_BASE_HEIGHT = 18.0f;				//< エネルギーゲージの基本縦幅(px)
    static constexpr float ENG_OFFSET_Y = -60.0f;					//< 下中央基準からのY軸上方オフセット

    // --- テクスチャアセット固有の解像度サイズ ---
    static constexpr float GAUGE_TEX_WIDTH = 2475.0f;				//< 各バー・背景テクスチャの元画像の横幅
    static constexpr float GAUGE_TEX_HEIGHT = 330.0f;				//< 各バー・背景テクスチャの元画像の縦幅
    static constexpr float FRAME_TEX_WIDTH = 2497.0f;				//< 枠外フレームテクスチャの元画像の横幅
    static constexpr float FRAME_TEX_HEIGHT = 357.0f;				//< 枠外フレームテクスチャの元画像の縦幅

	// --- 内部変数 ---
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_frame;		//< ゲージ枠テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hpBar;		//< HPバーテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_energyBar;	//< エネルギーバーテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_back;		//< 背景テクスチャ
};