/*****************************************************************//**
 * @file    StageManager.h
 * @brief   ステージの形状管理、壁・地面との衝突判定、およびレイキャストによる視線通し判定の制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

#include <WICTextureLoader.h>
#include <string>

class Robot;

// ステージ管理クラス
// OBBによる壁の当たり判定とデバッグ描画を行う
class StageManager
{
public:
	// コンストラクタ
	StageManager();
	// デストラクタ
	~StageManager();
	// 初期化処理
	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int stageNumber);
	// 通常描画
	void Render(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);
	// デバッグ描画 (OBBの枠線)
	void RenderDebug(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);

	// レイ（線）が壁に当たるかチェックする関数
	bool RayCast(const DirectX::SimpleMath::Vector3& start, const DirectX::SimpleMath::Vector3& dir,
		float length, float* outDist = nullptr);
	// 当たり判定 (OBB vs 球)
	void CheckCollision(Robot* robot);
	// 地面の高さ取得
	float GetGroundHeight(const DirectX::SimpleMath::Vector3& pos) const;
	// CSVファイルから当たり判定を読み込む
	void LoadCollisionFromCSV(const std::string& filename);

	// カメラのクロスヘアが指しているステージ内の正確な3D座標を割り出す
	DirectX::SimpleMath::Vector3 GetCameraAimPosition(const DirectX::SimpleMath::Vector3& aimOriginPos,
		const DirectX::SimpleMath::Vector3& aimDir, float maxDistance);
	// カメラの位置が壁にめり込まないように補正する
	DirectX::SimpleMath::Vector3 AdjustCameraPosition(const DirectX::SimpleMath::Vector3& targetPos,
		const DirectX::SimpleMath::Vector3& idealCameraPos);
	// 2本のレイ（中心・頭上）を用いて、壁や天井にめり込まない安全なカメラ距離を計算する
	float CalculateSafeCameraDistance(const DirectX::SimpleMath::Vector3& targetPos,
		const DirectX::SimpleMath::Vector3& idealCameraPos);

	// ステートがステージ情報を取得できるようにする
	StageManager* GetStageManager() const { return m_stageManager; }

private:
	// --- 初期化用補助関数 ---
	// モデルやテクスチャの読み込み
	void LoadModelsAndResources(ID3D11Device* device, ID3D11DeviceContext* context, int stageNumber);
	// デバッグ表示用オブジェクトの作成
	void SetupDebugPrimitives(ID3D11DeviceContext* context);
	// エリア制限ボックスの設定
	void SetupAreaLimits();
	// 壁の当たり判定の設定
	void SetupCollisionWalls(int stageNumber);

	// --- 描画補助関数 ---
	// ステージモデルの描画
	void RenderStageModel(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);
	// エリア境界（格子壁）の描画
	void RenderGridWalls(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);

	// --- 衝突判定の補助関数 ---
	// 地面・段差の判定
	void ResolveGroundCollision(Robot* robot, DirectX::SimpleMath::Vector3& pos,
		DirectX::SimpleMath::Vector3& velocity, float radius);
	// 壁との押し出し判定
	void ResolveWallCollision(DirectX::SimpleMath::Vector3& pos, 
		DirectX::SimpleMath::Vector3& velocity, float radius);
	// エリア制限
	void ApplyAreaLimits(DirectX::SimpleMath::Vector3& pos);

	// --- 接地判定の補助関数 ---
	// 疑似円柱（多角柱）を生成して壁リストに追加する関数
	void CreatePseudoCylinder(const DirectX::SimpleMath::Vector3& center,
		float radius, float height, int segments);

private:
	// --- 定数パラメータ ---
	static constexpr float PLAYER_RADIUS = 1.0f;                         //< プレイヤーの当たり判定半径
	static constexpr float ENEMY_RADIUS = 1.0f;                          //< 敵の当たり判定半径
	static constexpr float EPSILON = 0.0001f;                            //< 衝突判定の許容誤差
	static constexpr float EPSILON_SQ = 0.001f;                          //< 衝突判定の許容誤差の二乗
	static constexpr float STEP_HEIGHT = 0.5f;                           //< 段差として乗り越えられる高さ
	static constexpr float GROUND_RAY_LENGTH = 2.5f;                     //< 接地判定のレイの長さ
	static constexpr float RAY_ORIGIN_OFFSET_Y = 1.0f;                   //< 接地判定の開始オフセット
	static constexpr float GROUND_SNAP_RANGE = 0.5f;                     //< 接地スナップの許容範囲
	static constexpr float WALL_SLOPE_RATIO = 2.0f;                      //< 壁の角での跳ね上がり防止係数
	static constexpr float HALF_RATIO = 0.5f;                            //< 中心や半分（50%）を計算するための割合
	static constexpr float GROUND_CHECK_OFFSET_Y = 0.1f;                 //< 地面判定球を少し下げるためのオフセット値
	static constexpr float GROUND_CHECK_MARGIN = 1.0f;                   //< 頭上の壁を足場と誤認しないための高さ
	static constexpr float CYLINDER_WIDTH_MARGIN = 1.05f;                //< 擬似円柱の隙間を埋めるための幅拡張係数
	static constexpr float CAMERA_COLLISION_RADIUS = 0.5f;               //< カメラの当たり判定半径
	static constexpr float CAMERA_OVERHEAD_CLEARANCE = 1.0f;             //< カメラ上部の安全マージン
	static constexpr float GROUND_RAY_START_OFFSET_Y = 1.0f;             //< 接地判定時のレイ開始Yオフセット

	// エリア制限範囲の定義
	static constexpr float LIMIT_X_PLUS = 95.0f;                         //< X軸 右側の限界
	static constexpr float LIMIT_X_MINUS = 100.0f;                       //< X軸 左側の限界
	static constexpr float LIMIT_Z_PLUS = 100.0f;                        //< Z軸 奥側の限界
	static constexpr float LIMIT_Z_MINUS = 100.0f;                       //< Z軸 手前側の限界
	static constexpr float LIMIT_Y_CEILING = 38.0f;                      //< キャラクターが移動できる(高度制限)
	static constexpr float WALL_RENDER_HEIGHT = 46.0f;                   //< 画像の壁の(見た目の高さ)
	static constexpr float LIMIT_Y_FLOOR = 0.0f;                         //< 床の高さ (0m)

	// 描画（グリッド壁・デバッグ）用定数
	static constexpr float GRID_WALL_OFFSET = 1.2f;                      //< グリッド壁を描画する際の外側へのオフセット距離
	static constexpr float GRID_UV_TILE = 3.0f;                          //< グリッド壁テクスチャのUVタイリング数
	static constexpr float GRID_ALPHA = 0.4f;                            //< グリッド壁の透明度
	static constexpr unsigned int BLEND_SAMPLE_MASK = 0xFFFFFFFF;        //< 描画時のブレンド用サンプルマスク
	static constexpr float RAY_RENDER_THICKNESS = 0.05f;                 //< デバッグ描画時のレイキャスト線の太さ

	// --- クラス内部変数 ---
	std::unique_ptr<DirectX::Model> m_stageModel;                        //< ステージモデル
	std::vector<DirectX::BoundingOrientedBox> m_walls;                   //< 壁の判定データ群
	std::unique_ptr<DirectX::GeometricPrimitive> m_debugBox;             //< デバッグ描画用の箱モデル
	DirectX::BoundingBox m_limitBox;                                     //< デバッグ描画用のエリア制限ボックス (AABB)

	// レイキャストのデバッグ表示用変数
	DirectX::SimpleMath::Vector3 m_lastRayStart;                                                //< レイの開始点
	DirectX::SimpleMath::Vector3 m_lastRayEnd;                                                  //< レイの終点
	bool m_isRayHit;                                                                            //< ヒットフラグ

	StageManager* m_stageManager;                                                               //< 自身のポインタ

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_gridTexture;                             //< グリッド用テクスチャ
	std::unique_ptr<DirectX::BasicEffect> m_gridEffect;                                         //< グリッド用エフェクト

	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>> m_primitiveBatch;  //< プリミティブバッチ
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;                                    //< 入力レイアウト
};