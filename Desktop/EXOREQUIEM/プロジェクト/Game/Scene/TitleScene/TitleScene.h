/*****************************************************************//**
 * @file    TitleScene.h
 * @brief   ゲーム起動後のタイトル画面、ロゴ表示および入力によるシーン遷移制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Scene/SceneManager.h"
#include "Game/Common/UserResources.h"
#include "Game/UI/Menu/MenuUIManager.h"

 // ゲーム起動後のタイトル画面、ロゴ表示および入力によるシーン遷移制御
class TitleScene : public Scene<UserResources>
{
public:
	// メニューの種類
	enum class MenuType { Start, Quit, Max };

	// 16バイトアライメントを付与した定数バッファ構造体
	struct alignas(16) EyeGlowCB
	{
		DirectX::SimpleMath::Vector2 eyeUV;                                         //< ピクセル座標
		float uvRadius;                                                             //< ピクセルでの半径
		float time;                                                                 //< 経過時間
	};

	// コンストラクタ
	TitleScene();
	// デストラクタ
	~TitleScene() = default;
	// 初期化処理
	void Initialize() override;
	// 更新処理
	void Update(float elapsedTime) override;
	// 描画処理
	void Render() override;
	// 終了処理
	void Finalize() override;

	// デバイス依存リソースの作成
	void CreateDeviceDependentResources() override;
	// ウィンドウサイズ依存リソースの作成
	void CreateWindowSizeDependentResources() override;
	// デバイスロスト時の処理
	void OnDeviceLost() override;

private:
	// ウィンドウリサイズ監視と再生成
	void HandleWindowResize();
	// 決定操作の検出と遷移実行
	void HandleMenuExecution();
	// テクスチャリソースのロード
	void LoadAssets(ID3D11Device* device);

private:
	// --- 設定パラメータ（定数化） ---
	static constexpr float BASE_RESOLUTION_H = 1080.0f;                             //< UI基準縦解像度
	static constexpr float BLINK_INTERVAL = 0.5f;                                   //< 点滅間隔
	static constexpr float BLINK_ALPHA_VALUE = 0.5f;                                //< 点滅時の透明度
	static constexpr float BASE_TEXT_HEIGHT = 60.0f;                                //< 基準文字高
	static constexpr float FRAME_PADDING_RATIO = 1.2f;                              //< 選択枠の余白倍率
	static constexpr float SELECTED_EFFECT_SCALE = 1.1f;                            //< 選択中の拡大倍率
	static constexpr float HALF_RATIO = 0.5f;                                       //< 中心や半分（50%）を計算するための割合

	// メニュー配置比率
	static constexpr float MENU_BASE_X_RATIO = 0.48f;                               //< メニューのベースX配置比率
	static constexpr float MENU_X_OFFSET = 30.0f;                                   //< 解像度毎のX軸微調整オフセット
	static constexpr float MENU_Y_OFFSET = 60.0f;                                   //< 解像度毎のY軸微調整オフセット
	static constexpr float MENU_START_Y_RATIO = 0.65f;                              //< メニュー1項目目のY配置比率
	static constexpr float MENU_STEP_PADDING_RATIO = 1.5f;                          //< 文字高に対する行間の隙間倍率
	static constexpr float MENU_SPACING_MULTIPLIER = 1.6f;                          //< 項目間の縦間隔を決める乗算値
	static constexpr float MENU_FRAME_SCALE_X = 0.35f;                              //< メニュー選択枠の横スケール
	static constexpr float MENU_FRAME_SCALE_Y = 0.3f;                               //< メニュー選択枠の縦スケール

	// 背景シェーダー演出（EyeGlow）用の設定パラメータ
	static constexpr float BG_TEXTURE_WIDTH = 1920.0f;                              //< 背景画像の元横幅
	static constexpr float BG_TEXTURE_HEIGHT = 1080.0f;                             //< 背景画像の元高さ
	static constexpr float EYE_PIXEL_X = 950.0f;                                    //< 背景画像内での目の中心X座標
	static constexpr float EYE_PIXEL_Y = 390.0f;                                    //< 背景画像内での目の中心Y座標
	static constexpr float EYE_GLOW_RADIUS = 70.0f;                                 //< 光の半径（画像内のピクセル数）

	// レンダリング用のトリック描画パラメータ
	static constexpr UINT TRICK_VERTEX_COUNT = 3;                                   //< 全画面描画に使う頂点数
	static constexpr UINT TRICK_VERTEX_OFFSET = 0;                                  //< 頂点バッファの開始オフセット

	// --- クラス内部変数 ---
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;                            //< スプライトバッチ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_titleTexture;                //< タイトルロゴテクスチャ

	float m_time;                                                                   //< 演出用の経過時間
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_eyeGlowPS;                          //< 自作ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_timeConstantBuffer;                      //< 定数バッファ
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_eyeGlowVS;                         //< 自作頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;                      //< サンプラーステート

	// --- メニュー制御用 ---
	MenuUIManager m_menuManager;                                                    //< メニュー管理コンポーネント
	DirectX::Mouse::ButtonStateTracker m_mouseTracker;                              //< マウスステートトラッカー

	float m_blinkTimer;                                                             //< 点滅タイマー
	bool m_showText;                                                                //< テキスト表示フラグ

	// --- レイアウト・共通リソース ---
	DirectX::SimpleMath::Vector2 m_titlePosition;                                   //< タイトル位置
	float m_titleScale;                                                             //< タイトルスケール
	RECT m_backgroundRect;                                                          //< 背景矩形
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_frameTexture;                //< 選択枠テクスチャ
	DirectX::SimpleMath::Vector2 m_frameSize;                                       //< 選択枠サイズ
};