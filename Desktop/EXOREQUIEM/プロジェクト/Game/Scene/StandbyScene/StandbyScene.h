/*****************************************************************//**
 * @file    StandbyScene.h
 * @brief   出撃待機画面（機体展示・メニュー選択）の挙動と描画
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include "Game/Scene/SceneManager.h"
#include "Game/Common/UserResources.h"
#include "Game/UI/Menu/MenuUIManager.h"

 // 出撃待機画面（機体展示・メニュー選択）を管理するシーンクラス
class StandbyScene : public Scene<UserResources>
{
public:
	// メニューの種類
	enum class MenuType { Select, HowToPlay, Title, Max };
	// ステージ選択の種類
	enum class StageMenuType { Training, Stage1, Stage2, Stage3, Max };
	// シーンの現在の状態
	enum class SceneState { MainMenu, StageSelect };

	// コンストラクタ
	StandbyScene();
	// デストラクタ
	~StandbyScene() = default;
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
	// チュートリアル表示中の入力待ち処理
	bool HandleTutorialInput();
	// 決定操作の検出とメニュー項目の実行
	void HandleMenuExecution();
	// ステージ選択の決定処理
	void HandleStageExecution();

	// カメラ行列とタイマーの更新
	void UpdateCamera(float elapsedTime);
	// メニューのレイアウト再計算
	void UpdateMenuLayout(float W, float H);

	// 背景の描画
	void DrawBackground();
	// 展示用プレイヤーモデル（3D）の描画
	void DrawPlayerModel();
	// 操作説明画像の描画（表示中のみ）
	void DrawHowToPlay();
	// 操作ガイド描画関数
	void DrawGuideUI();

	// モデルリソースのロード
	void LoadModel(ID3D11Device* device);
	// テクスチャリソースのロード
	void LoadAssets(ID3D11Device* device);

private:
	// --- UI・メニュー配置パラメータ ---
	static constexpr float BASE_RESOLUTION_H = 1080.0f;                          //< UI基準縦解像度
	static constexpr float TARGET_HEIGHT = 70.0f;                                //< メニュー項目の基準高さ
	static constexpr float FRAME_PADDING_RATIO = 1.2f;                           //< 選択枠の余白倍率
	static constexpr float SELECTED_EFFECT_SCALE = 1.1f;                         //< 選択中の拡大倍率
	static constexpr float MENU_BASE_X_RATIO = 0.2f;                             //< メニュー配置X開始位置
	static constexpr float MENU_BASE_Y_RATIO = 0.2f;                             //< メニュー配置Y開始位置
	static constexpr float MENU_FRAME_SCALE_X = 0.5f;                            //< メニュー選択枠の横スケール
	static constexpr float MENU_FRAME_SCALE_Y = 0.3f;                            //< メニュー選択枠の縦スケール
	static constexpr float MENU_LINE_SPACING = 150.0f;                           //< メニュー項目間の縦の隙間（行間）
	static constexpr float STAGE_MENU_LINE_SPACING = 120.0f;                     //< ステージメニュー項目間の縦の隙間
	static constexpr float STAGE_MENU_Y_OFFSET = 100.0f;                         //< ステージメニュー縦間隔の追加オフセット

	// --- チュートリアル・ガイドUIパラメータ ---
	static constexpr float TUTORIAL_IMAGE_SCALE_RATIO = 0.8f;                                   //< 画像の画面高に対する比率
	static constexpr float GUIDE_MARGIN = 20.0f;                                                //< ガイドUIの画面端からの余白ピクセル
	static constexpr float GUIDE_TARGET_HEIGHT = 100.0f;                                        //< ガイドUIの表示目標高さ
	static inline const DirectX::SimpleMath::Color GUIDE_COLOR = { 1.0f, 1.0f, 1.0f, 0.9f };    //< ガイドUIの描画色（透明度90%）

	// --- 3D表示・配置に関する定数 ---
	static constexpr float MODEL_RENDER_SCALE = 0.1f;                            //< プレイヤーモデルの描画スケール
	static constexpr float MODEL_ROTATION_SPEED = 1.0f;                          //< モデルの回転速度倍率
	static constexpr float MODEL_POSITION_X = 1.5f;                              //< モデルのX配置位置
	static constexpr float MODEL_POSITION_Y = 0.0f;                              //< モデルのY配置位置
	static constexpr float MODEL_POSITION_Z = 0.0f;                              //< モデルのZ配置位置

	// --- カメラ・レンダリング設定の定数 ---
	static inline const DirectX::SimpleMath::Vector3 CAMERA_POS = { 0.0f, 0.8f, 5.0f };         //< カメラの位置
	static inline const DirectX::SimpleMath::Vector3 CAMERA_TARGET = { 0.0f, 0.8f, 0.0f };      //< カメラの注視点
	static constexpr float CAMERA_NEAR_PLANE = 0.1f;                                            //< カメラのニアクリップ面
	static constexpr float CAMERA_FAR_PLANE = 1000.0f;                                          //< カメラのファークリップ面

	// --- 各種比率・クリア値・初期化値の定数化 ---
	static constexpr float HALF_RATIO = 0.5f;                                    //< 中心や半分（50%）を計算するための割合
	static constexpr float CLEAR_DEPTH_VALUE = 1.0f;                             //< 奥行きバッファの初期化値（最奥）
	static constexpr uint8_t CLEAR_STENCIL_VALUE = 0;                            //< ステンシルバッファの初期化値
	static constexpr float SPRITE_ROTATION_NONE = 0.0f;                          //< スプライトの無回転
	static constexpr float SPRITE_ORIGIN_X_LEFT = 0.0f;                          //< スプライトの左端原点
	static constexpr LONG RECT_ORIGIN_COORD = 0;                                 //< 矩形の原点座標 (0)
	static constexpr int CURSOR_VISIBLE_THRESHOLD = 0;                           //< カーソル表示の判定しきい値

	// --- クラス内部変数 ---
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;                         //< スプライトバッチ

	// --- 3D表示用 ---
	std::unique_ptr<DirectX::Model> m_playerModel;                               //< プレイヤーの3Dモデル
	DirectX::SimpleMath::Matrix m_view;                                          //< カメラの視点
	DirectX::SimpleMath::Matrix m_proj;                                          //< 画面の歪み設定
	float m_timer = 0.0f;                                                        //< 演出用タイマー

	// --- メニュー制御用 ---
	MenuUIManager m_mainMenuManager;                                             //< メインメニューコンポーネント
	MenuUIManager m_stageMenuManager;                                            //< ステージ選択メニューコンポーネント

	DirectX::Mouse::ButtonStateTracker m_mouseTracker;                           //< マウスステートトラッカー
	DirectX::Keyboard::KeyboardStateTracker m_keyboardTracker;                   //< キーボードトラッカー

	bool m_showCursor = true;                                                    //< カーソル表示フラグ
	bool m_isShowingHowToPlay = false;                                           //< 操作説明画像を表示中か

	// --- 2D描画用 ---
	RECT m_backgroundRect = {};                                                  //< 背景描画矩形
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_backgroundTexture;        //< 背景テクスチャ

	// チュートリアル画像
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_howToPlayTexture;         //< 操作説明画像テクスチャ
	DirectX::SimpleMath::Vector2 m_sizeHowToPlayImage;                           //< 操作説明画像サイズ

	// 共通UI
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_frameTexture;             //< 選択枠テクスチャ
	DirectX::SimpleMath::Vector2 m_frameSize;                                    //< 選択枠サイズ

	float m_guiScale = 1.0f;                                                     //< GUIスケール

	// ステージ選択制御用
	SceneState m_currentState = SceneState::MainMenu;                            //< シーンの現在の状態

	// 操作ガイド用テクスチャとサイズ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_guideMenuTex;             //< メインメニュー用ガイドテクスチャ
	DirectX::SimpleMath::Vector2 m_sizeGuideMenu;                                //< メインメニュー用ガイド画像サイズ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_guideStageTex;            //< ステージセレクト用ガイドテクスチャ
	DirectX::SimpleMath::Vector2 m_sizeGuideStage;                               //< ステージセレクト用ガイド画像サイズ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_guideHowtoplayTex;        //< 説明書用ガイドテクスチャ
	DirectX::SimpleMath::Vector2 m_sizeGuideHowtoplay;                           //< 説明書用ガイド画像サイズ
};