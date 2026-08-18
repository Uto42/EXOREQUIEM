/*****************************************************************//**
 * @file    GameLoadingScreen.h
 * @brief   ロード画面の描画および読み込み状況の進行表示
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Scene/SceneManager.h"
#include "Game/Common/UserResources.h"

class GameLoadingScreen : public LoadingScreen<UserResources>
{
public:
	// コンストラクタ
	GameLoadingScreen();
	// デストラクタ
	~GameLoadingScreen();
	// 初期化処理
	void Initialize() override;
	// 更新処理
	void Update(float elapsedTime) override;
	// 描画処理
	void Render() override;
	// 終了処理
	void Finalize() override;

	// デバイスリソース作成
	void CreateDeviceDependentResources() override;
	// ウィンドウサイズ依存リソース作成
	void CreateWindowSizeDependentResources() override;
	// デバイスロスト時処理
	void OnDeviceLost() override;

private:
	// --- 調整用定数パラメータ ---
	static constexpr float BASE_WIDTH = 1280.0f;                             //< 基準幅
	static constexpr float BASE_HEIGHT = 720.0f;                             //< 基準高さ
	static constexpr float ROTATION_SPEED = DirectX::XM_PI;                  //< アイコンの回転速度 (1秒で180度)
	static constexpr float ICON_BASE_SCALE_RATIO = 0.3f;                     //< 背景に対するアイコンの基準スケール比率
	static constexpr float ICON_POSITION_X_RATIO = 0.9f;                     //< アイコンの画面X配置比率
	static constexpr float ICON_POSITION_Y_RATIO = 0.85f;                    //< アイコンの画面Y配置比率
	static constexpr float HALF_RATIO = 0.5f;                                //< 中心や半分（50%）を計算するため
	static const DirectX::SimpleMath::Vector2 BG_POSITION;                   //< 背景位置定数

	// --- 状態値・パラメータ ---
	float m_rotationAngle = 0.0f;                                            //< 回転角度
	float m_bgScale = 1.0f;                                                  //< 動的背景スケール
	float m_iconScale = 1.0f;                                                //< 動的アイコンスケール
	DirectX::SimpleMath::Vector2 m_iconOrigin;                               //< アイコンの中心座標
	DirectX::SimpleMath::Vector2 m_bgPosition;                               //< 背景座標
	DirectX::SimpleMath::Vector2 m_iconPosition;                             //< アイコン配置位置
	static bool s_isBooting;                                                 //< 起動時かどうかを判別する静的フラグ

	// --- リソース ---
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch = nullptr;           //< スプライトバッチ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_loadingIconTexture;   //< ロードアイコン
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_loadingTexture;       //< 背景テクスチャ
};