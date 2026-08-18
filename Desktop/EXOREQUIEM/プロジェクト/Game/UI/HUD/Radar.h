/*****************************************************************//**
 * @file    Radar.h
 * @brief   プレイヤー周辺の敵位置を表示するUIレーダーの管理
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <d3d11.h>

 /**
  * @class Radar
  * @brief 敵の位置を2D座標に変換し、レーダーUIとして描画するクラス
  */
class Radar
{
public:
	// コンストラクタ
	Radar() = default;
	// デストラクタ
	~Radar() = default;
	// 初期化処理
    void Initialize(ID3D11Device* device);
	// 更新処理
    void Update(float dt);
	// 描画処理
    void Render(
        DirectX::SpriteBatch* batch, 
        const DirectX::SimpleMath::Vector2& screenPos,
        float scaledRadius,
        const DirectX::SimpleMath::Vector3& playerPos, 
        float playerYaw,
        const std::vector<DirectX::SimpleMath::Vector3>& enemyPositions );

private:
    // レーダー外枠（背景）の描画
    void DrawRadarBackground(
        DirectX::SpriteBatch* batch,
        const DirectX::SimpleMath::Vector2& screenPos, 
        float scaledRadius);
    // スイープ線（くるくる）の描画
    void DrawSweepLine(
        DirectX::SpriteBatch* batch,
        const DirectX::SimpleMath::Vector2& screenPos, 
        float scaledRadius);

    // 敵アイコンの描画
    void DrawEnemies(
        DirectX::SpriteBatch* batch,
        const DirectX::SimpleMath::Vector2& screenPos,
        float scaledRadius, 
        const DirectX::SimpleMath::Vector3& playerPos,
        float playerYaw,
        const std::vector<DirectX::SimpleMath::Vector3>& enemyPositions);
    // プレイヤー（自機）アイコンの描画
    void DrawPlayer(
        DirectX::SpriteBatch* batch,
        const DirectX::SimpleMath::Vector2& screenPos,
        float scaledRadius);

private:
    // --- スケール・画面基準の定数 ---
    static constexpr float HALF_RATIO = 0.5f;               // 半分を求める係数

    // --- レーダー設定定数 ---
    static constexpr float RADAR_RADIUS = 100.0f;           //< 画面上の基本半径(px)
    static constexpr float DETECT_RANGE = 100.0f;           //< レーダーに映る現実（ワールド空間）の最大索敵距離
    static constexpr float SWEEP_SPEED = 3.0f;              //< スイープ線の回転速度

    // --- 背景テクスチャ情報 ---
    static constexpr float RADAR_TEX_SIZE = 1050.0f;        //< 背景テクスチャの幅・高さ
    static constexpr float RADAR_TEX_CENTER = 525.0f;       //< 背景テクスチャの中心座標

    // --- スイープ線（くるくる）の演出設定 ---
    static constexpr int   TRAIL_SEGMENTS = 80;             //< 残像の分割数
    static constexpr float TRAIL_ANGLE_WIDTH = 2.0f;        //< 残像の全体の長さ
    static constexpr float SWEEP_ALPHA_MAX = 0.35f;         //< スイープ線の最大透明度
    static constexpr float SWEEP_LENGTH_RATIO = 0.92f;      //< レーダー半径に対する線の長さの割合

    // --- マーカー（ドット）の基準サイズ ---
    static constexpr float ENEMY_DOT_SIZE = 8.0f;           //< 敵ドットの基準サイズ
    static constexpr float PLAYER_DOT_SIZE = 10.0f;         //< 自機ドットの基準サイズ
    static constexpr float ENEMY_DISPLAY_LIMIT= 0.9f;       //< レーダー枠内に収めるための安全表示限界

    float m_sweepAngle = 0.0f;                                       //< くるくる回る線の角度

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whitePixel;   //< 1x1の白いテクスチャ（点や線の描画用）

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_radarTexture; //< レーダー背景画像
};