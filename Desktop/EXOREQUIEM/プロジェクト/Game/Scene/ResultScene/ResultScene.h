/*****************************************************************//**
 * @file    ResultScene.h
 * @brief   戦闘結果（リザルト）画面の表示、クリアランクの計算とスコア集計
 *
 * @author  名前
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Scene/SceneManager.h"
#include "Game/Common/UserResources.h"
#include "Game/UI/Menu/MenuUIManager.h"

 // ゲームの勝敗を表示し、タイトル画面への遷移を待つリザルトシーン
class ResultScene : public Scene<UserResources>
{
public:
	// コンストラクタ
	ResultScene();
	// デストラクタ
	~ResultScene() = default;
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

	// --- 共有パラメータ ---
	static bool s_isGameClear;                                                 //< true: ゲームクリア, false: ゲームオーバー
	static float s_clearTime;                                                  //< クリアにかかった時間
	static float s_remainingHP;                                                //< 残りHP
	static float s_maxHP;                                                      //< 最大HP

private:
	// マウスカーソルやシステム状態の更新
	void UpdateSystemState();
	// 決定操作の検出とメニュー項目の実行
	void HandleMenuExecution();
	// 点滅などの演出用タイマー更新
	void UpdateAnimation(float elapsedTime);
	// ランク計算
	void CalculateRank();

	// 背景の描画
	void DrawBackground();
	// リザルトヘッダー（Complete / Failed）の描画
	void DrawResultHeader();
	// ランク結果の描画
	void DrawRank();
	// 操作ガイドUIの描画
	void DrawGuideUI();

	// テクスチャリソースのロード
	void LoadAssets(ID3D11Device* device);

private:
	// --- 静的定数パラメータの集約 ---
	static constexpr float BASE_RESOLUTION_H = 1080.0f;                        //< UI基準縦解像度
	static constexpr float BLINK_INTERVAL = 0.5f;                              //< 点滅間隔 (秒)
	static constexpr float MENU_BASE_HEIGHT = 70.0f;                           //< メニュー項目の基準高さ
	static constexpr float FRAME_PADDING_RATIO = 1.2f;                         //< 選択枠の余白倍率
	static constexpr float SELECTED_EFFECT_SCALE = 1.1f;                       //< 選択中の拡大演出倍率

	// 画面配置の割合 (Y軸・X軸)
	static constexpr float HEADER_Y_RATIO = 0.15f;                             //< ヘッダー表示位置 (上部15%)
	static constexpr float RANK_Y_RATIO = 0.45f;                               //< ランク表示位置 (中央45%)
	static constexpr float RANK_LABEL_X_RATIO = 0.42f;                         //< ランクタイトルのX位置 (左寄り42%)
	static constexpr float RANK_VALUE_X_RATIO = 0.58f;                         //< ランク文字のX位置 (右寄り58%)
	static constexpr float MENU_START_Y_RATIO = 0.7f;                          //< メニュー開始位置 (下部70%)
	static constexpr float MENU_STEP_Y_RATIO = 0.12f;                          //< メニュー項目間の縦間隔 (12%)

	// ターゲットサイズ表示高
	static constexpr float HEADER_TARGET_HEIGHT = 120.0f;                      //< クリアロゴの表示高さ
	static constexpr float RANK_LABEL_TARGET_HEIGHT = 80.0f;                   //< 「RANK」ロゴの表示高さ
	static constexpr float RANK_VALUE_TARGET_HEIGHT = 300.0f;                  //< ランク文字(S/A/B)の表示高さ

	// UI配置・スケール用の定数
	static constexpr float MENU_FRAME_SCALE_X = 0.5f;                          //< メニュー選択枠の横スケール
	static constexpr float MENU_FRAME_SCALE_Y = 0.3f;                          //< メニュー選択枠の縦スケール
	static constexpr float MENU_SPACING_MULTIPLIER = 1.6f;                     //< メニューの縦間隔を調整する倍率
	static constexpr float GUIDE_MARGIN = 20.0f;                               //< ガイドUIの画面端からの余白ピクセル
	static constexpr float GUIDE_TARGET_HEIGHT = 100.0f;                       //< ガイドUIの表示目標高さ
	static constexpr float GUIDE_ALPHA = 0.9f;                                 //< ガイドUIの透明度
	static constexpr float HALF_RATIO = 0.5f;                                  //< 中心や半分（50%）を計算するための割合

	// ランク評価用のタイマー・HP割合基準
	static constexpr float RANK_S_TIME_LIMIT = 60.0f;                          //< Sランクに必要なクリアタイム（秒以下）
	static constexpr float RANK_S_HP_RATIO = 0.7f;                             //< Sランクに必要なHP割合（以上）
	static constexpr float RANK_A_TIME_LIMIT = 120.0f;                         //< Aランクに必要なクリアタイム（秒以下）
	static constexpr float RANK_A_HP_RATIO = 0.4f;                             //< Aランクに必要なHP割合（以上）
	static constexpr float RANK_B_TIME_LIMIT = 360.0f;                         //< Bランクに必要なクリアタイム（秒以下）
	static constexpr float RANK_B_HP_RATIO = 0.01f;                            //< Bランクに必要なHP割合（以上）

	static const DirectX::SimpleMath::Vector2 POS_RANK;                        //< ランクの配置位置基準定数

	// --- メニュー制御（型定義） ---
	enum class MenuType { Retry, Return, Max };

	// --- クラス内部変数 ---
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;                       //< スプライトバッチ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_resultTexture;          //< リザルトテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_backgroundTexture;      //< 背景テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_guideTex;               //< 操作ガイドテクスチャ
	RECT m_backgroundRect;                                                     //< 背景描画矩形

	float m_blinkTimer;                                                        //< 点滅用タイマー
	bool m_showText;                                                           //< テキスト表示フラグ
	char m_finalRank;                                                          //< 最終ランク文字
	std::wstring m_rankText;                                                   //< ランクテキスト
	std::wstring m_detailText;                                                 //< 詳細テキスト
	DirectX::SimpleMath::Vector4 m_rankColor;                                  //< ランク色

	MenuUIManager m_menuManager;                                               //< メニュー管理コンポーネント
	DirectX::Mouse::ButtonStateTracker m_mouseTracker;                         //< マウストラッカー

	// --- 画像リソース ---
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texClear;               //< クリア画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texGameOver;            //< ゲームオーバー画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_frameTexture;           //< 選択枠画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texRankLabel;           //< ランクタイトル画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texRankS;               //< ランクS画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texRankA;               //< ランクA画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texRankB;               //< ランクB画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texRankC;               //< ランクC画像

	// --- 画像サイズ ---
	DirectX::SimpleMath::Vector2 m_sizeClear;                                  //< クリア画像サイズ
	DirectX::SimpleMath::Vector2 m_sizeGameOver;                               //< ゲームオーバー画像サイズ
	DirectX::SimpleMath::Vector2 m_frameSize;                                  //< 選択枠サイズ
	DirectX::SimpleMath::Vector2 m_sizeRankLabel;                              //< ランクタイトルサイズ
	DirectX::SimpleMath::Vector2 m_sizeRankS;                                  //< ランクSサイズ
	DirectX::SimpleMath::Vector2 m_sizeRankA;                                  //< ランクAサイズ
	DirectX::SimpleMath::Vector2 m_sizeRankB;                                  //< ランクBサイズ
	DirectX::SimpleMath::Vector2 m_sizeRankC;                                  //< ランクCサイズ
	DirectX::SimpleMath::Vector2 m_sizeGuide;                                  //< 操作ガイド画像サイズ
};