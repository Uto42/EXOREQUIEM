/*****************************************************************//**
 * @file    Thruster.h
 * @brief   機体スラスターの視覚効果：移動入力に応じた噴射エフェクトの動的生成および描画制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#pragma once

 /**
  * @class Thruster
  * @brief 機体各所に配置されたスラスターの表示・出力を一括管理するクラス
  */
class Thruster
{
public:
	/**
	 * @struct ThrusterConfig
	 * @brief  スラスター1個分の個別設定データ
	 */
	struct ThrusterConfig
	{
		DirectX::SimpleMath::Vector3 offset;                              //< 取り付け位置
		DirectX::SimpleMath::Vector3 direction;                           //< 噴射方向
		bool isMain = false;                                              //< メインスラスターかどうか
		float currentIntensity = 0.0f;                                    //< 現在の出力
	};

	// コンストラクタ
	Thruster();
	// デストラクタ
	~Thruster();
	// 初期化
	void Initialize(ID3D11Device* device);
	// 更新処理
	void Update(float dt, bool isMoving, bool isBack);
	// 描画処理
	void Render(
		ID3D11DeviceContext* context, 
		const DirectX::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view, 
		const DirectX::SimpleMath::Matrix& proj, 
		const DirectX::SimpleMath::Vector3& playerPos,
		const DirectX::SimpleMath::Quaternion& playerRot);

private:
	// 単体スラスターのワールド行列計算
	DirectX::SimpleMath::Matrix CalculateThrusterWorldMatrix(
		const ThrusterConfig& config, 
		const DirectX::SimpleMath::Vector3& playerPos, 
		const DirectX::SimpleMath::Quaternion& playerRot) const;

	// --- 調整用定数パラメータ ---
	static constexpr float UV_WRAP_BOUND = 100.0f;                    //< UVオーバーフロー防止のラップ境界値
	static constexpr float UV_SCALE_Y = 0.5f;                         //< テクスチャのV方向タイリング倍率
	static constexpr float EFFECT_DEFAULT_ALPHA = 0.8f;               //< エフェクトの基本不透明度
	static constexpr float RENDER_MIN_INTENSITY = 0.01f;              //< 描画をスキップする最小出力しきい値（最適化用）
	static constexpr float MAX_EFFECT_LENGTH = 0.4f;                  //< 最大出力時のエフェクトの長さ
	static constexpr float EFFECT_WIDTH = 0.1f;                       //< エフェクトの横幅（太さ）
	static const DirectX::SimpleMath::Vector3 THRUSTER_COLOR;         //< スラスター色 (R, G, B)
	static constexpr float UV_SCROLL_SPEED = 5.0f;                    //< UVスクロール速度
	static constexpr float INTENSITY_LERP_SPEED = 10.0f;              //< 出力変化速度
	static const DirectX::SimpleMath::Vector3 MAIN_THRUSTER_BASE_DIR; //< メインスラスターの未正規化噴射方向
	static const DirectX::SimpleMath::Vector3 MODEL_BASE_DIR;         //< スラスターモデル自体の基準方向

	// --- メインスラスターの配置座標 ---
	static constexpr float MAIN_THRUSTER_OFFSET_X = 0.17f;            //< メインスラスター配置座標X
	static constexpr float MAIN_THRUSTER_OFFSET_Y = 1.41f;            //< メインスラスター配置座標Y
	static constexpr float MAIN_THRUSTER_OFFSET_Z = -0.5f;            //< メインスラスター配置座標Z

	// --- 出力値の範囲定数 ---
	static constexpr float INTENSITY_MIN = 0.0f;                      //< 最小出力値（消灯）
	static constexpr float INTENSITY_MAX = 1.0f;                      //< 最大出力値（全開）

	// --- 状態値・パラメータ ---
	std::vector<ThrusterConfig> m_thrusters;                          //< スラスターの構成リスト
	float m_uvOffset = 0.0f;                                          //< テクスチャスクロール用オフセット

	// --- リソース ---
	std::unique_ptr<DirectX::Model> m_model;                          //< スラスター用3Dモデル
	std::unique_ptr<DirectX::DGSLEffect> m_effect;                    //< 描画用エフェクト
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;          //< 入力レイアウト
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;       //< スラスター用テクスチャ
};