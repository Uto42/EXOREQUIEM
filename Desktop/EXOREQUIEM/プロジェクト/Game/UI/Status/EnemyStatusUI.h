/*****************************************************************//**
 * @file    EnemyStatusUI.h
 * @brief   敵ユニットのHP情報を3D座標から2Dスクリーンへ変換して表示するHUD制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

class Enemy;

/**
 * @brief 敵HP表示UIクラス
 */
class EnemyStatusUI
{
public:
	// コンストラクタ
	EnemyStatusUI();
	// デストラクタ
	~EnemyStatusUI();
	// 初期化処理
	void Initialize(ID3D11Device* device);
	// 描画処理
	void Render(DirectX::SpriteBatch* spriteBatch, const std::vector<std::unique_ptr<Enemy>>& enemies,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj, const D3D11_VIEWPORT& viewport);

private:
	// ボス専用のHPゲージを描画位置を計算して描画
	void RenderBossHealthBar(
		DirectX::SpriteBatch* spriteBatch, 
		Enemy* boss,
		float guiScale, 
		const D3D11_VIEWPORT& viewport);

	// 通常の敵のHPゲージをワールド座標から変換して描画
	void RenderNormalEnemyHealthBar(
		DirectX::SpriteBatch* spriteBatch,
		Enemy* enemy,
		float guiScale, 
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& proj, 
		const D3D11_VIEWPORT& viewport);

	// 個別の敵HPゲージを描画
	void DrawHealthBar(
		DirectX::SpriteBatch* spriteBatch, 
		const DirectX::SimpleMath::Vector2& hpPos,
		const DirectX::SimpleMath::Vector2& hpDrawScale, 
		const DirectX::SimpleMath::Vector2& hpSize,
		float health, float maxHealth);

private:
	// --- スケール・画面基準の定数 ---
	static constexpr float HALF_RATIO = 0.5f;				//< 半分を求める係数
	static constexpr float BASE_SCREEN_HEIGHT = 1080.0f;    //< 基準とする画面解像度（縦）

	// --- ゲージ配置・基本サイズ定数 ---
	static constexpr float Y_OFFSET = 3.5f;					//< 敵のワールド座標から頭上へのオフセット距離
	static constexpr float BASE_BAR_WIDTH = 180.0f;			//< HPゲージの基本横幅(px)
	static constexpr float BASE_BAR_HEIGHT = 20.0f;			//< HPゲージの基本縦幅(px)

	// --- ボス専用ゲージ定数 ---
	static constexpr float BOSS_BAR_WIDTH_RATIO = 0.55f;	//< ボスHPバーの横幅（画面幅に対する比率）
	static constexpr float BOSS_BAR_HEIGHT_RATIO = 1.5f;	//< ボスHPバーの縦幅（基本サイズに対する倍率）
	static constexpr float BOSS_BAR_POS_Y_RATIO = 0.08f;	//< ボスHPバーのY配置位置（画面高に対する比率）

	// --- 描画判定定数 ---
	static constexpr float MIN_DEPTH = 0.0f;				//< 画面手前の深度クリップ値
	static constexpr float MAX_DEPTH = 1.0f;				//< 画面奥の深度クリップ値

	// --- テクスチャアセット固有の解像度サイズ ---
	static constexpr float HP_TEX_WIDTH = 2475.0f;			//< HPバー・背景テクスチャの元画像の横幅
	static constexpr float HP_TEX_HEIGHT = 330.0f;			//< HPバー・背景テクスチャの元画像の縦幅
	static constexpr float FRAME_TEX_WIDTH = 2497.0f;		//< 枠外フレームテクスチャの元画像の横幅
	static constexpr float FRAME_TEX_HEIGHT = 357.0f;		//< 枠外フレームテクスチャの元画像の縦幅

	static constexpr uint32_t WHITE_PIXEL = 0xFFFFFFFF;		//< 白テクスチャ用のピクセルデータ

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureWhite; //< 白色テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_frame;        //< 枠テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_hpBar;        //< HPバーテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_back;         //< 背景テクスチャ

};