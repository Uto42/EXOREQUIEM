/*****************************************************************//**
 * @file    InGameUI.h
 * @brief   ゲームプレイ中の情報表示（HP、エネルギー、残弾数、ステート情報）の描画制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <d3d11.h>
#include "Radar.h"

 // 前方宣言
class World;
class LockOnSystem;
class Player;
class Enemy;
class Robot;
class Camera;
class LockOnUI;

/**
 * @class InGameUI
 * @brief ゲーム内UI描画および更新を管理するクラス
 */
class InGameUI
{
public:
	// コンストラクタ
	InGameUI();
	// デストラクタ
	~InGameUI();

	// 初期化処理
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context,
		World* world, Camera* camera, LockOnSystem* lockOnSystem, LockOnUI* lockOnUI);
	// 更新処理
	void Update(float dt, const RECT& screenSize, const DirectX::SimpleMath::Viewport& viewport);
	// 描画処理
	void Render();

	// デバイス関連のリソース作成
	void CreateDeviceDependentResources(ID3D11Device* device);
	// 画面サイズ関連のリソース作成（スケール計算）
	void CreateWindowSizeDependentResources(const RECT& screenSize);

	// トレーニングモードかどうかをセットする
	void SetTrainingMode(bool isTraining) { m_isTrainingMode = isTraining; }

private:
	// --- データ構造 ---
	// 数字1つ分のデータ構造
	struct DigitRect
	{
		float x;    //< 画像内の開始X座標
		float w;    //< 数字の幅
	};

	// --- 計算・ユーティリティ補助関数 ---
	// カメラのYaw（Y軸回転）を計算する
	float CalculateCameraYaw() const;

	// --- 更新補助関数 ---
	// プレイヤーのステータス情報の更新
	void UpdatePlayerStatus(const Player* player);
	// 武器の残弾数情報の更新
	void UpdateWeaponStatus(const Player* player);

	// --- 描画補助関数 ---
	// 数字の描画ヘルパー
	void DrawNumber(int number, DirectX::SimpleMath::Vector2 position, float scale);
	// バーの描画ヘルパー
	void DrawValue(int value, DirectX::SimpleMath::Vector2 position, float scale);
	// プレイヤー情報の描画
	void DrawPlayerInfo();
	// 武器アイコンと残弾数の描画
	void DrawWeaponIconsAndAmmo();
	// 敵情報の描画
	void DrawEnemyInfo() const;
	// レーダーのデータ準備と描画
	void DrawRadar(float cameraYaw);
	// ガイド描画ヘルパー
	void DrawGuides();
	// リロードバーの描画ヘルパー
	void DrawReloadBars();

private:
	// --- スケール・画面基準の定数 ---
	static constexpr float BASE_SCREEN_HEIGHT = 1080.0f;            //< GUIスケールの基準となる画面の高さ
	static constexpr float BASE_ICON_HEIGHT = 720.0f;               //< アイコンスケールの基準となる高さ
	static constexpr float HALF_RATIO = 0.5f;                       //< 半分を求める係数

	// --- レーダー描画用のレイアウト定数 ---
	static constexpr float RADAR_BASE_RADIUS = 180.0f;              //< レーダーの基本半径
	static constexpr float RADAR_MARGIN = 20.0f;                    //< 画面端からの余白

	// --- 武器・残弾数アイコンのレイアウト微調整定数 ---
	static constexpr float AMMO_NUMBER_SHIFT_X = 125.0f;            //< 数字を右に寄せるオフセット
	static constexpr float GUN_ICON_OFFSET_X = 200.0f;              //< マシンガンアイコンの数字からのXズレ
	static constexpr float GUN_ICON_OFFSET_Y = 20.0f;               //< マシンガンアイコンの数字からのYズレ
	static constexpr float SHOTGUN_ICON_OFFSET_X = 220.0f;          //< ショットガンアイコンの数字からのXズレ
	static constexpr float SHOTGUN_ICON_OFFSET_Y = 20.0f;			//< ショットガンアイコンの数字からのYズレ
	static constexpr float MISSILE_ICON_OFFSET_X = 220.0f;          //< ミサイルアイコンの数字からのXズレ
	static constexpr float MISSILE_ICON_OFFSET_Y = 35.0f;           //< ミサイルアイコンの数字からのYズレ

	// --- スケール・描画設定定数 ---
	static constexpr float SCALE_GUN_BASE = 0.15f;                  //< マシンガンアイコンの基本スケール
	static constexpr float SCALE_SHOTGUN_BASE = 0.15f;				//< ショットガンアイコンの基本スケール
	static constexpr float SCALE_MISSILE_BASE = 0.09f;              //< ミサイルアイコンの基本スケール
	static constexpr float AMMO_TEXT_SCALE = 0.3f;                  //< 残弾数数字の基本スケール
	static constexpr float NUMBER_SPACING = 10.0f;                  //< 数字同士の文字間隔

	// --- 数字フォント画像の元サイズ ---
	static constexpr float NUMBER_TEX_WIDTH = 1739.0f;              //< 数字テクスチャの元横幅
	static constexpr float NUMBER_TEX_HEIGHT = 386.0f;              //< 数字テクスチャの元高さ

	// --- レイアウト割合定数 ---
	static constexpr float LAYOUT_LEFT = 0.02f;                     //< 左余白比率
	static constexpr float LAYOUT_STATE_Y = 0.08f;                  //< プレイヤー状態Y比率
	static constexpr float LAYOUT_HP_Y = 0.12f;                     //< プレイヤーHPバーY比率
	static constexpr float LAYOUT_ENERGY_Y = 0.16f;                 //< プレイヤーエネルギーバーY比率
	static constexpr float LAYOUT_ENEMY_X = 0.65f;                  //< 敵情報X比率
	static constexpr float LAYOUT_ENEMY_HP_Y = 0.06f;               //< 敵HPバーY比率
	static constexpr float LAYOUT_ENEMY_STATE_Y = 0.10f;            //< 敵状態Y比率
	static constexpr float LAYOUT_AMMO_X = 0.9f;                    //< 残弾数X比率
	static constexpr float LAYOUT_MISSILE_Y = 0.90f;                //< ミサイル情報Y比率
	static constexpr float LAYOUT_GUN_Y = 0.78f;                    //< 銃情報Y比率

	// --- リロードバー描画用のレイアウト定数 ---
	static constexpr float RELOAD_BAR_OFFSET_X = 120.0f;             //< 照準中心からの左右グループの離れ幅
	static constexpr float RELOAD_BAR_OFFSET_Y = 0.0f;               //< 照準中心からの上下オフセット（全バー共通）
	static constexpr float RELOAD_BAR_SPACING_X = 10.0f;             //< 横に並べるバー同士の間隔
	static constexpr float RELOAD_BAR_SCALE_BASE = 0.2f;             //< バー全体の基本拡大倍率
	static constexpr float RELOAD_BAR_LOW_AMMO_THRESHOLD = 0.2f;     //< 残弾警告の閾値（20%）
	static constexpr float RELOAD_BAR_INACTIVE_ALPHA = 0.6f;         //< 非アクティブ武器の透明度

	// --- アイコン・テキスト設定定数 ---
	static constexpr float ICON_SCALE = 0.15f;                       //< アイコン基本スケール
	static constexpr float ICON_TEXT_OFFSET = 38.0f;                 //< アイコンからテキストへの横オフセット
	static constexpr float ICON_TEXT_Y_OFFSET = 6.0f;                //< アイコンからテキストへの縦オフセット

	// --- ガイドUI定数 ---
	static constexpr float GUIDE_SCALE = 0.2f;						 //< 画面端の操作ガイド表示スケール
	static constexpr float GUIDE_MARGIN_X = 40.0f;					 //< ガイドUIの左端からの余白ピクセル
	static constexpr float GUIDE_MARGIN_Y = 50.0f;					 //< ガイドUIの上端からの余白ピクセル
	static constexpr float GUIDE_SPACING = 20.0f;					 //< 複数のガイドUIを縦に並べる際の間隔

	// --- ガイドUIリソース ---
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texEscGuide;       //< ESCガイドテクスチャ
	DirectX::SimpleMath::Vector2 m_sizeEscGuide;                          //< ESCガイドテクスチャのサイズ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texTrainingGuide;  //< トレーニングガイドテクスチャ
	DirectX::SimpleMath::Vector2 m_sizeTrainingGuide;                     //< トレーニングガイドテクスチャのサイズ
	bool m_isTrainingMode = false;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texReloadFrame;	  //< リロードフレームテクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texReloadBar;	  //< リロードバー用テクスチャ
	DirectX::SimpleMath::Vector2 m_sizeReloadBar;						  //< リロードバー用テクスチャのサイズ

	// --- 描画リソース ---
	std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;                        //< スプライトバッチ
	std::unique_ptr<DirectX::SpriteFont> m_spriteFont;                          //< スプライトフォント
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texNumbers;              //< 数字用テクスチャ

	// --- クロスヘア (照準) ---
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_crosshairTexture;        //< 照準テクスチャ
	DirectX::XMUINT2 m_crosshairSize;                                           //< 照準の画像サイズ
	DirectX::SimpleMath::Vector2 m_crosshairPosition;                           //< 照準の描画位置
	bool m_showCrosshair;                                                       //< 照準の表示フラグ

	// --- 武器アイコン ---
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_gunIconTexture;          //< 銃アイコン
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_missileIconTexture;      //< ミサイルアイコン
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shotgunIconTexture;		//< ショットガンアイコン

	// --- プレイヤー情報 ---
	float m_currentEnergy;                                     //< 現在のエネルギー
	float m_maxEnergy;                                         //< 最大エネルギー
	int m_missileAmmo;                                         //< ミサイル残弾
	int m_gunAmmo;                                             //< 銃残弾
	float m_playerHP;                                          //< プレイヤー現在のHP
	float m_playerMaxHP;                                       //< プレイヤー最大HP
	wchar_t m_playerStateString[64];                           //< プレイヤーの状態文字列

	// --- 敵情報 ---
	bool m_enemyActive;                                        //< 敵情報の表示有効フラグ
	float m_enemyHP;                                           //< ターゲットの現在HP
	float m_enemyMaxHP;                                        //< ターゲットの最大HP
	wchar_t m_enemyStateText[64];                              //< ターゲットの状態文字列

	// --- システム・外部参照 ---
	Radar m_radar;                                             //< レーダー機能本体
	World* m_world;                                            //< エンティティ管理者への参照
	Camera* m_camera;                                          //< カメラへの参照
	LockOnSystem* m_lockOnSystem;                              //< ロックオンシステムへの参照
	ID3D11DeviceContext* m_context;                            //< デバイスコンテキスト
	LockOnUI* m_lockOnUI;									   //< ロックオンUIの参照

	// --- 座標・レイアウト状態 ---
	RECT m_screenRect;                                         //< 画面サイズ保存用矩形
	DirectX::SimpleMath::Vector3 m_playerPos;                  //< プレイヤーのワールド座標
	float m_playerYaw;                                         //< プレイヤーの向き（ラジアン）
	float m_guiScale;                                          //< 画面解像度に基づくスケーリング倍率
	DirectX::SimpleMath::Vector2 m_numberSize;                 //< 数字テクスチャのサイズ情報
	int m_currentWeaponType = 0;							   //< 現在の武器タイプ

	// --- 動的に計算される描画座標 ---
	DirectX::SimpleMath::Vector2 m_posPlayerState;             //< プレイヤー状態表示座標
	DirectX::SimpleMath::Vector2 m_posPlayerHP;                //< プレイヤーHP表示座標
	DirectX::SimpleMath::Vector2 m_posPlayerEnergy;            //< プレイヤーエネルギー表示座標
	DirectX::SimpleMath::Vector2 m_posEnemyHP;                 //< 敵HP表示座標
	DirectX::SimpleMath::Vector2 m_posEnemyState;              //< 敵状態表示座標
	DirectX::SimpleMath::Vector2 m_posAmmoMissile;             //< ミサイル残弾数表示座標
	DirectX::SimpleMath::Vector2 m_posAmmoGun;                 //< マシンガン残弾数表示座標
	DirectX::SimpleMath::Vector2 m_posGunIcon;                 //< マシンガンアイコン描画座標
	DirectX::SimpleMath::Vector2 m_posShotgunIcon;			   //< ショットガンアイコン描画座標
	DirectX::SimpleMath::Vector2 m_posMissileIcon;             //< ミサイルアイコン描画座標

	float m_gunIconScale;                                      //< マシンガンアイコンのスケール
	float m_shotgunIconScale;								   //< ショットガンアイコンのスケール
	float m_missileIconScale;                                  //< ミサイルアイコンのスケール
};