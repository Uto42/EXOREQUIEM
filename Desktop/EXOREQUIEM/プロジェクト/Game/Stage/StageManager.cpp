/*****************************************************************//**
 * @file    StageManager.cpp
 * @brief   ステージの形状管理、壁・地面との衝突判定、およびレイキャストによる視線通し判定の制御
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Stage/StageManager.h"
#include "Game/GameObjects/Robot/Robot.h"
#include "Game/Systems/Effect/EffectSystem.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include <sstream>

/**
 * @brief コンストラクタ
 */
StageManager::StageManager()
	: m_lastRayStart(DirectX::SimpleMath::Vector3::Zero)
	, m_lastRayEnd(DirectX::SimpleMath::Vector3::Zero)
	, m_isRayHit(false)
	, m_stageManager(this) // 自身を参照できるようにthisポインタを設定
{
}

/**
 * @brief デストラクタ
 */
StageManager::~StageManager()
{
}

/**
 * @brief 初期化処理
 * @param[in] device ID3D11Device
 * @param[in] context ID3D11DeviceContext
 * @param[in] stageNumber 読み込むステージの番号 (0=Tutorial, 1=Stage1, 2=Stage2, 3=Stage3)
 */
void StageManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int stageNumber)
{
	// モデル、テクスチャ、エフェクトの読み込みと設定
	LoadModelsAndResources(device, context, stageNumber);

	// デバッグ用プリミティブの作成
	SetupDebugPrimitives(context);

	// マップのエリア制限ボックスの設定
	SetupAreaLimits();

	// 衝突判定用の壁（OBB）と柱の配置
	SetupCollisionWalls(stageNumber);
}

/**
 * @brief モデル、テクスチャ、エフェクトの読み込みと設定
 * @param[in] device ID3D11Device
 * @param[in] context ID3D11DeviceContext
 * @param[in] stageNumber 読み込むステージの番号
 */
void StageManager::LoadModelsAndResources(ID3D11Device* device, ID3D11DeviceContext* context, int stageNumber)
{
	DirectX::EffectFactory effectFactory(device);
	effectFactory.SetDirectory(L"Resources/Models/Stage");

	// ステージ番号に応じたモデルパスを決定
	std::wstring modelPath;
	switch (stageNumber)
	{
	case 0:
		modelPath = L"Resources/Models/Stage/Tutorial.sdkmesh";
		break;
	case 1:
		modelPath = L"Resources/Models/Stage/Stage.sdkmesh";
		break;
	case 2:
		modelPath = L"Resources/Models/Stage/Stage2.sdkmesh";
		break;
	case 3:
		modelPath = L"Resources/Models/Stage/Stage3.sdkmesh";
		break;
	default:
		modelPath = L"Resources/Models/Stage/Stage.sdkmesh";
		break;
	}

	// SDKMESHモデルの読み込み
	m_stageModel = DirectX::Model::CreateFromSDKMESH(device, modelPath.c_str(), effectFactory);

	DX::ThrowIfFailed(
		DirectX::CreateWICTextureFromFile(device, L"Resources/Textures/InGame/inGame_grid_wall.png",
			nullptr, m_gridTexture.GetAddressOf())
	);

	// グリッド描画用のBasicEffectを作成
	m_gridEffect = std::make_unique<DirectX::BasicEffect>(device);
	m_gridEffect->SetTextureEnabled(true);
	m_gridEffect->SetTexture(m_gridTexture.Get());

	m_primitiveBatch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionTexture>>(context);

	// 入力レイアウトの作成
	void const* shaderByteCode;
	size_t byteCodeLength;
	m_gridEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
	device->CreateInputLayout(DirectX::VertexPositionTexture::InputElements,
		DirectX::VertexPositionTexture::InputElementCount,
		shaderByteCode, byteCodeLength,
		m_inputLayout.GetAddressOf());

	for (auto& mesh : m_stageModel->meshes)
	{
		for (auto& part : mesh->meshParts)
		{
			auto basicEffect = dynamic_cast<DirectX::BasicEffect*>(part->effect.get());
			if (basicEffect)
			{
				// ボス用ステージ（暗い工場など）では、テクスチャが真っ暗にならないよう環境光を調整する
				if (stageNumber == 3)
				{
					basicEffect->SetLightingEnabled(true);
					basicEffect->SetAmbientLightColor(DirectX::Colors::White);
					basicEffect->SetVertexColorEnabled(false);
				}
				else
				{
					basicEffect->SetLightingEnabled(false);
					basicEffect->SetAmbientLightColor(DirectX::Colors::White);
					basicEffect->SetVertexColorEnabled(false);
				}

				part->CreateInputLayout(device, basicEffect, part->inputLayout.ReleaseAndGetAddressOf());
			}
		}
	}
}

/**
 * @brief デバッグ用プリミティブの作成
 * @param[in] context ID3D11DeviceContext
 */
void StageManager::SetupDebugPrimitives(ID3D11DeviceContext* context)
{
	m_debugBox = DirectX::GeometricPrimitive::CreateBox(context, DirectX::SimpleMath::Vector3(1.0f));
}

/**
 * @brief マップのエリア制限ボックスの設定
 */
void StageManager::SetupAreaLimits()
{
	// 制限ボックスの中心と半径を計算
	float centerX = (LIMIT_X_PLUS - LIMIT_X_MINUS) * HALF_RATIO;
	float extentX = (LIMIT_X_PLUS + LIMIT_X_MINUS) * HALF_RATIO;
	float centerZ = (LIMIT_Z_PLUS - LIMIT_Z_MINUS) * HALF_RATIO;
	float extentZ = (LIMIT_Z_PLUS + LIMIT_Z_MINUS) * HALF_RATIO;

	// AABBの中心と半径を計算してBoundingBoxを作成
	DirectX::SimpleMath::Vector3 center(centerX, (LIMIT_Y_CEILING + LIMIT_Y_FLOOR) * HALF_RATIO, centerZ);
	DirectX::SimpleMath::Vector3 extents(extentX, (LIMIT_Y_CEILING - LIMIT_Y_FLOOR) * HALF_RATIO, extentZ);
	m_limitBox = DirectX::BoundingBox(center, extents);
}

/**
 * @brief 衝突判定用の壁（OBB）と柱の配置
 * @param[in] stageNumber 読み込むステージの番号
 */
void StageManager::SetupCollisionWalls(int stageNumber)
{
	m_walls.clear();

	std::string csvPath;
	switch (stageNumber)
	{
	case 0:
		csvPath = "Resources/Data/tutorial_collision.csv";
		break;
	case 1:
		csvPath = "Resources/Data/stage1_collision.csv";
		break;
	case 2:
		csvPath = "Resources/Data/stage2_collision.csv";
		break;
	case 3:
		csvPath = "Resources/Data/stage3_collision.csv";
		break;
	default:
		csvPath = "Resources/Data/stage1_collision.csv";
		break;
	}

	LoadCollisionFromCSV(csvPath);
}

/**
 * @brief CSVから当たり判定データをロードする
 * @param[in] filename 読み込むCSVファイルのパス
 */
void StageManager::LoadCollisionFromCSV(const std::string& filename)
{
	std::ifstream file(filename);
	std::string line;

	while (std::getline(file, line))
	{
		// 空行やコメント行をスキップ
		if (line.empty() || line[0] == '#') continue;

		// ヘッダー行をスキップ
		if (line.find("type") != std::string::npos) continue;

		std::stringstream stringStream(line);
		std::string cellString;
		std::vector<float> parsedValues;

		while (std::getline(stringStream, cellString, ','))
		{
			// 余計な空白や改行コードを排除
			cellString.erase(0, cellString.find_first_not_of(" \t\r\n"));
			cellString.erase(cellString.find_last_not_of(" \t\r\n") + 1);

			// 空のセルをスキップ
			if (cellString.empty()) continue;

			char* endPointer = nullptr;
			float parsedValue = std::strtof(cellString.c_str(), &endPointer);

			// 変換に成功した（文字列の最後まで数値だった）場合のみ追加
			if (endPointer != cellString.c_str()) {
				parsedValues.push_back(parsedValue);
			}
		}

		// 最低8個のパラメータ（種類, X, Y, Z, 幅, 高さ, 奥行, 回転）がない行は無視
		if (parsedValues.size() < 8) continue;

		int type = static_cast<int>(parsedValues[0]);
		DirectX::SimpleMath::Vector3 position(parsedValues[1], parsedValues[2], parsedValues[3]);
		DirectX::SimpleMath::Vector3 extents(parsedValues[4], parsedValues[5], parsedValues[6]);
		float rotationAngle = parsedValues[7];

		if (type == 0) // 普通の箱 (Wall)
		{
			DirectX::BoundingOrientedBox box;
			box.Center = position;
			box.Extents = extents; // CSVの幅・高さ・奥行きが「半分のサイズ」として入っている前提
			box.Orientation = 
				DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::Up, rotationAngle);
			m_walls.push_back(box);
		}
		else if (type == 1) // 擬似円柱 (Cylinder)
		{
			CreatePseudoCylinder(position, parsedValues[4], parsedValues[5], static_cast<int>(parsedValues[7]));
		}
	}
}

/**
 * @brief カメラのクロスヘアが指している正確なステージ内の座標を割り出す
 * @param[in] aimOriginPos カメラの位置
 * @param[in] aimDir カメラの視線方向（正規化されていることが前提）
 * @param[in] maxDistance 最大射程距離
 * @return 視線がオブジェクトや地面と交差する3D座標
 */
DirectX::SimpleMath::Vector3 StageManager::GetCameraAimPosition(const DirectX::SimpleMath::Vector3& aimOriginPos,
	const DirectX::SimpleMath::Vector3& aimDir, float maxDistance)
{
	float closestDistance = maxDistance;
	bool isHitAnything = false;

	// ステージ内の障害物（壁や柱）に当たるかチェック
	float wallDistance = 0.0f;
	if (RayCast(aimOriginPos, aimDir, maxDistance, &wallDistance))
	{
		closestDistance = wallDistance;
		isHitAnything = true;
	}

	// 壁に当たらなかった場合、地面（Y = 0の無限平面）との数学的交差をチェック
	if (!isHitAnything)
	{
		DirectX::SimpleMath::Plane groundPlane(DirectX::SimpleMath::Vector3::Up, 0.0f);
		DirectX::SimpleMath::Ray cameraRay(aimOriginPos, aimDir);
		float groundDistance = 0.0f;

		// 視線が地面と交差し、かつ前方の最大射程内であれば採用
		if (cameraRay.Intersects(groundPlane, groundDistance))
		{
			if (groundDistance > 0.0f && groundDistance <= maxDistance)
			{
				closestDistance = groundDistance;
				isHitAnything = true;
			}
		}
	}

	// すべてをすり抜けた場合（真上を向いた場合など）は、最大射程距離の座標を返す
	return aimOriginPos + (aimDir * closestDistance);
}

/**
 * @brief カメラの位置が壁にめり込まないように補正する
 * @param[in] targetPos カメラが注目している対象（プレイヤーの頭など）の座標
 * @param[in] idealCameraPos 障害物がない場合の理想のカメラ座標
 * @return 補正後のカメラ座標
 */
DirectX::SimpleMath::Vector3 StageManager::AdjustCameraPosition(const DirectX::SimpleMath::Vector3& targetPos,
	const DirectX::SimpleMath::Vector3& idealCameraPos)
{
	// 理想のカメラ位置とターゲット位置の間の方向ベクトルを計算
	DirectX::SimpleMath::Vector3 direction = idealCameraPos - targetPos;
	float originalDistance = direction.Length();

	if (originalDistance < EPSILON) return idealCameraPos;
	direction.Normalize();

	// 天井や壁のすっぽ抜けを考慮した安全な距離を取得
	float safeDistance = CalculateSafeCameraDistance(targetPos, idealCameraPos);

	// 壁や天井に当たっていたら、カメラの半径分だけ手前に引いてめり込みを防ぐ
	if (safeDistance < originalDistance)
	{
		float adjustedDistance = std::max(0.0f, safeDistance - CAMERA_COLLISION_RADIUS);
		return targetPos + direction * adjustedDistance;
	}

	return idealCameraPos;
}

/**
 * @brief 2本のレイ（中心・頭上）を用いて、壁や天井にめり込まない安全なカメラ距離を計算する
 * @param[in] targetPos カメラが注目している対象の座標
 * @param[in] idealCameraPos 障害物がない場合の理想のカメラ座標
 * @return 障害物にヒットした距離（ヒットしなければ本来の距離）
 */
float StageManager::CalculateSafeCameraDistance(const DirectX::SimpleMath::Vector3& targetPos, 
	const DirectX::SimpleMath::Vector3& idealCameraPos)
{
	DirectX::SimpleMath::Vector3 mainDirection = idealCameraPos - targetPos;
	float mainMaxDistance = mainDirection.Length();
	if (mainMaxDistance < EPSILON) return mainMaxDistance;
	mainDirection.Normalize();

	// --- 1本目：通常のカメラ中心への判定 ---
	float mainHitDistance = mainMaxDistance;
	bool isMainHit = RayCast(targetPos, mainDirection, mainMaxDistance, &mainHitDistance);

	// --- 2本目：カメラの「頭上」への天井すっぽ抜け防止判定 ---
	DirectX::SimpleMath::Vector3 idealOverheadPos = 
		idealCameraPos + DirectX::SimpleMath::Vector3(0.0f, CAMERA_OVERHEAD_CLEARANCE, 0.0f);

	DirectX::SimpleMath::Vector3 overheadDirection = idealOverheadPos - targetPos;

	float overheadMaxDistance = overheadDirection.Length();
	float overheadHitDistance = overheadMaxDistance;

	bool isOverheadHit = false;

	if (overheadMaxDistance > EPSILON)
	{
		overheadDirection.Normalize();
		isOverheadHit = RayCast(targetPos, overheadDirection, overheadMaxDistance, &overheadHitDistance);
	}

	// 両方の結果を統合して、より手前でヒットした方を安全距離として採用する
	float finalSafeDistance = mainMaxDistance;

	if (isMainHit)
	{
		finalSafeDistance = mainHitDistance;
	}

	if (isOverheadHit)
	{
		// 2本目（頭上）が当たった場合、その距離の割合をメインの視線軸の長さに換算する
		float hitRatio = overheadHitDistance / overheadMaxDistance;
		float projectedDistance = mainMaxDistance * hitRatio;

		if (projectedDistance < finalSafeDistance)
		{
			finalSafeDistance = projectedDistance;
		}
	}

	return finalSafeDistance;
}

/**
 * @brief 通常描画
 * @param[in] context ID3D11DeviceContext
 * @param[in] states CommonStates
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void StageManager::Render(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	// ステージモデルの描画
	RenderStageModel(context, states, view, proj);

	// エリア境界（格子壁）の描画
	RenderGridWalls(context, states, view, proj);
}

/**
 * @brief ステージモデルの描画
 * @param[in] context ID3D11DeviceContext
 * @param[in] states CommonStates
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void StageManager::RenderStageModel(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	if (m_stageModel)
	{
		m_stageModel->Draw(context, *states, DirectX::SimpleMath::Matrix::Identity, view, proj);
	}
}

/**
 * @brief エリア境界（格子壁）の描画
 * @param[in] context ID3D11DeviceContext
 * @param[in] states CommonStates
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void StageManager::RenderGridWalls(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	if (m_gridTexture)
	{
		context->RSSetState(states->CullNone());
		context->OMSetBlendState(states->AlphaBlend(), nullptr, BLEND_SAMPLE_MASK);
		context->OMSetDepthStencilState(states->DepthRead(), 0);
		context->IASetInputLayout(m_inputLayout.Get());

		// グリッド描画用のBasicEffectを設定
		m_gridEffect->SetView(view);
		m_gridEffect->SetProjection(proj);
		m_gridEffect->SetDiffuseColor(DirectX::Colors::White);
		m_gridEffect->SetAlpha(GRID_ALPHA);

		m_gridEffect->Apply(context);

		float limitXPlusOffset = LIMIT_X_PLUS + GRID_WALL_OFFSET;
		float limitXMinusOffset = LIMIT_X_MINUS + GRID_WALL_OFFSET;
		float limitZPlusOffset = LIMIT_Z_PLUS + GRID_WALL_OFFSET;
		float limitZMinusOffset = LIMIT_Z_MINUS + GRID_WALL_OFFSET;
		float wallHeight = WALL_RENDER_HEIGHT;
		float uvTiling = GRID_UV_TILE;

		m_primitiveBatch->Begin();

		// 奥 (Z+)
		m_primitiveBatch->DrawQuad(
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, 0.0f, limitZPlusOffset), DirectX::SimpleMath::Vector2(uvTiling, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, 0.0f, limitZPlusOffset), DirectX::SimpleMath::Vector2(0.0f, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, wallHeight, limitZPlusOffset), DirectX::SimpleMath::Vector2(0.0f, 0.0f)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, wallHeight, limitZPlusOffset), DirectX::SimpleMath::Vector2(uvTiling, 0.0f))
		);

		// 手前 (Z-)
		m_primitiveBatch->DrawQuad(
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, 0.0f, -limitZMinusOffset), DirectX::SimpleMath::Vector2(uvTiling, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, 0.0f, -limitZMinusOffset), DirectX::SimpleMath::Vector2(0.0f, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, wallHeight, -limitZMinusOffset), DirectX::SimpleMath::Vector2(0.0f, 0.0f)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, wallHeight, -limitZMinusOffset), DirectX::SimpleMath::Vector2(uvTiling, 0.0f))
		);

		// 左 (X-)
		m_primitiveBatch->DrawQuad(
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, 0.0f, limitZPlusOffset), DirectX::SimpleMath::Vector2(uvTiling, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, 0.0f, -limitZMinusOffset), DirectX::SimpleMath::Vector2(0.0f, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, wallHeight, -limitZMinusOffset), DirectX::SimpleMath::Vector2(0.0f, 0.0f)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				-limitXMinusOffset, wallHeight, limitZPlusOffset), DirectX::SimpleMath::Vector2(uvTiling, 0.0f))
		);

		// 右 (X+)
		m_primitiveBatch->DrawQuad(
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, 0.0f, -limitZMinusOffset), DirectX::SimpleMath::Vector2(uvTiling, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, 0.0f, limitZPlusOffset), DirectX::SimpleMath::Vector2(0.0f, uvTiling)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, wallHeight, limitZPlusOffset), DirectX::SimpleMath::Vector2(0.0f, 0.0f)),
			DirectX::VertexPositionTexture(DirectX::SimpleMath::Vector3(
				limitXPlusOffset, wallHeight, -limitZMinusOffset), DirectX::SimpleMath::Vector2(uvTiling, 0.0f))
		);

		m_primitiveBatch->End();

		context->RSSetState(states->CullCounterClockwise());
		context->OMSetDepthStencilState(states->DepthDefault(), 0);
	}
}

/**
 * @brief デバッグ描画
 * @param[in] context ID3D11DeviceContext
 * @param[in] states CommonStates
 * @param[in] view ビュー行列
 * @param[in] proj プロジェクション行列
 */
void StageManager::RenderDebug(ID3D11DeviceContext* context, const DirectX::CommonStates* states,
	const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(states);
	UNREFERENCED_PARAMETER(view);
	UNREFERENCED_PARAMETER(proj);

	if (!m_debugBox) return;

	// OBBのデバッグ描画（必要に応じて有効化）
	for (const auto& wall : m_walls)
	{
		const auto extents = DirectX::SimpleMath::Vector3(wall.Extents) * 2.0f;

		DirectX::SimpleMath::Matrix world =
			DirectX::SimpleMath::Matrix::CreateScale(extents)
			* DirectX::SimpleMath::Matrix::CreateFromQuaternion(wall.Orientation)
			* DirectX::SimpleMath::Matrix::CreateTranslation(wall.Center);

		// m_debugBox->Draw(world, view, proj, Colors::Red, nullptr, true);
	}

	// レイキャストの軌跡をデバッグ描画する
	if (m_lastRayStart != DirectX::SimpleMath::Vector3::Zero)
	{
		DirectX::SimpleMath::Vector3 rayVector = m_lastRayEnd - m_lastRayStart;
		float length = rayVector.Length();

		if (length > EPSILON)
		{
			DirectX::SimpleMath::Vector3 midPoint = (m_lastRayStart + m_lastRayEnd) * HALF_RATIO;
			DirectX::SimpleMath::Vector3 rayDirection = rayVector;
			rayDirection.Normalize();

			DirectX::SimpleMath::Vector3 rotationAxis = DirectX::SimpleMath::Vector3::Up.Cross(rayDirection);
			float dotProduct = DirectX::SimpleMath::Vector3::Up.Dot(rayDirection);
			DirectX::SimpleMath::Quaternion rotation = DirectX::SimpleMath::Quaternion::Identity;

			if (rotationAxis.LengthSquared() > EPSILON)
			{
				rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(rotationAxis, acos(dotProduct));
			}
			else if (dotProduct < 0.0f)
			{
				// 真下を向いている特殊ケース
				rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(
					DirectX::SimpleMath::Vector3::Right, DirectX::XM_PI);
			}

			// 細長い箱でレイの軌跡を表現する
			DirectX::SimpleMath::Matrix world =
				DirectX::SimpleMath::Matrix::CreateScale(RAY_RENDER_THICKNESS, length, RAY_RENDER_THICKNESS) 
				* DirectX::SimpleMath::Matrix::CreateFromQuaternion(rotation) 
				* DirectX::SimpleMath::Matrix::CreateTranslation(midPoint);

			DirectX::XMVECTOR color = m_isRayHit ? DirectX::Colors::Red : DirectX::Colors::Yellow;
			// m_debugBox->Draw(world, view, proj, color, nullptr, true);
		}
	}
}

/**
 * @brief レイ（線）が壁に当たるかチェックする
 * @param[in] start レイの開始点
 * @param[in] dir レイの方向ベクトル
 * @param[in] length レイの最大距離
 * @param[out] outDist ヒットした距離を格納するポインタ
 * @return ヒットしたかどうか
 */
bool StageManager::RayCast(const DirectX::SimpleMath::Vector3& start, 
	const DirectX::SimpleMath::Vector3& dir, float length, float* outDist)
{
	// 無効なベクトルやゼロベクトルの場合は処理しない
	if (std::isnan(dir.x) || std::isnan(dir.y) || std::isnan(dir.z) || dir.LengthSquared() < EPSILON_SQ)
	{
		if (outDist) *outDist = 0.0f;
		return false;
	}

	float minDistance = FLT_MAX;
	bool isHit = false;

	m_lastRayStart = start;
	m_lastRayEnd = start + dir * length;
	m_isRayHit = false;

	// 配置されているすべての壁とレイの交差判定を行う
	for (const auto& wall : m_walls)
	{
		float distance = 0.0f;
		if (wall.Intersects(start, dir, distance))
		{
			// 指定距離内で、かつ今までで一番近いヒットを記録する
			if (distance < length && distance < minDistance)
			{
				minDistance = distance;
				isHit = true;
			}
		}
	}

	if (isHit)
	{
		if (outDist) *outDist = minDistance;
		m_isRayHit = true;
		return true;
	}

	return false;
}

/**
 * @brief Robot共通の当たり判定（地面・壁・エリア制限）
 * @param[in] robot 判定を行うRobotのポインタ
 */
void StageManager::CheckCollision(Robot* robot)
{
	if (!robot) return;

	DirectX::SimpleMath::Vector3 position = robot->GetPosition();
	DirectX::SimpleMath::Vector3 velocity = robot->GetVelocity();
	float radius = robot->GetRadius();

	// 地面・段差の当たり判定とスナップ処理
	ResolveGroundCollision(robot, position, velocity, radius);
	// 壁の当たり判定と押し出し処理
	ResolveWallCollision(position, velocity, radius);
	// エリア制限ボックスの適用
	ApplyAreaLimits(position);

	// Robotの座標と速度を更新
	robot->SetPosition(position);
	robot->SetVelocity(velocity);
}

/**
 * @brief 地面および段差の当たり判定・スナップ処理
 * @param[in] robot ロボットのポインタ（接地フラグ更新用）
 * @param[in,out] pos ロボットの座標
 * @param[in,out] velocity ロボットの速度
 * @param[in] radius ロボットの当たり判定半径
 */
void StageManager::ResolveGroundCollision(Robot* robot, DirectX::SimpleMath::Vector3& pos,
	DirectX::SimpleMath::Vector3& velocity, float radius)
{
	// ロボットの足元の球で地面を判定する
	DirectX::BoundingSphere robotSphere(pos, radius);
	float maxGroundY = -FLT_MAX;
	bool isFoundGround = false;

	DirectX::BoundingSphere groundCheckSphere(
		pos + DirectX::SimpleMath::Vector3(0.0f, radius - GROUND_CHECK_OFFSET_Y, 0.0f), radius);

	// 足元の地面を探す
	for (const auto& wall : m_walls)
	{
		float boxTop = wall.Center.y + wall.Extents.y;
		float boxBottom = wall.Center.y - wall.Extents.y;

		// 大まかな高さチェック
		if (pos.y - radius > boxTop + GROUND_CHECK_MARGIN) continue;
		if (pos.y + radius < boxBottom) continue;

		if (!wall.Intersects(groundCheckSphere)) continue;

		float robotBottom = pos.y;

		// 乗り越えられる段差を考慮して地面に乗れるか判定
		if (robotBottom >= boxTop - STEP_HEIGHT)
		{
			if (boxTop > maxGroundY)
			{
				maxGroundY = boxTop;
				isFoundGround = true;
			}
		}
	}

	// 判定が見つからなくても、床の高さ付近まで落ちたらそこを地面とする
	if (!isFoundGround && pos.y <= LIMIT_Y_FLOOR + STEP_HEIGHT)
	{
		isFoundGround = true;
		maxGroundY = LIMIT_Y_FLOOR;
	}

	bool wasGrounded = robot->IsGrounded();
	robot->SetGrounded(false);

	if (isFoundGround)
	{
		float targetY = maxGroundY;

		// 高さが下回っているか、下降中でスナップ範囲内であれば地面に吸着させる
		if (pos.y < targetY || (pos.y - targetY < GROUND_SNAP_RANGE && velocity.y <= 0.0f))
		{
			pos.y = targetY;

			if (velocity.y <= 0.0f)
			{
				velocity.y = 0.0f;
				robot->SetGrounded(true);

				// 空中から着地した瞬間のみエフェクトを発生させる
				if (!wasGrounded)
				{
					DirectX::SimpleMath::Vector3 footPos = DirectX::SimpleMath::Vector3(pos.x, maxGroundY, pos.z);
					EffectSystem::Instance()->SpawnLandingDust(footPos);
				}
			}
		}
	}
}

/**
 * @brief 壁の当たり判定と押し出し処理
 * @param[in,out] pos ロボットの座標
 * @param[in,out] velocity ロボットの速度
 * @param[in] radius ロボットの当たり判定半径
 */
void StageManager::ResolveWallCollision(DirectX::SimpleMath::Vector3& pos, 
	DirectX::SimpleMath::Vector3& velocity, float radius)
{
	DirectX::BoundingSphere robotSphere(pos, radius);

	for (const auto& wall : m_walls)
	{
		if (!wall.Intersects(robotSphere)) continue;

		float boxTop = wall.Center.y + wall.Extents.y;

		// すでに箱の上に乗っている場合（接地判定で処理済み）は壁として押し出さない
		if (pos.y >= boxTop - 0.01f) continue;

		// OBBのローカル空間に変換して、最も近い点を求める
		DirectX::SimpleMath::Quaternion orientation = wall.Orientation;
		DirectX::SimpleMath::Quaternion inverseRotation;
		orientation.Inverse(inverseRotation);

		// ロボットの座標をOBBのローカル空間に変換
		DirectX::SimpleMath::Vector3 localPosition =
			DirectX::SimpleMath::Vector3::Transform(pos - wall.Center, inverseRotation);

		// OBBの境界内に収まるように座標をクランプして、最も近い点を求める
		DirectX::SimpleMath::Vector3 closestLocal;
		closestLocal.x = std::max(-wall.Extents.x, std::min(localPosition.x, wall.Extents.x));
		closestLocal.y = std::max(-wall.Extents.y, std::min(localPosition.y, wall.Extents.y));
		closestLocal.z = std::max(-wall.Extents.z, std::min(localPosition.z, wall.Extents.z));

		DirectX::SimpleMath::Vector3 pushVectorLocal = localPosition - closestLocal;
		pushVectorLocal.y = 0.0f;

		float distanceSquared = pushVectorLocal.LengthSquared();

		// 完全に埋まっている場合の救済処理
		if (distanceSquared <= EPSILON)
		{
			float distanceX = wall.Extents.x - fabsf(localPosition.x);
			float distanceZ = wall.Extents.z - fabsf(localPosition.z);
			if (distanceX < distanceZ)
			{
				pushVectorLocal = (localPosition.x > 0.0f)
					? DirectX::SimpleMath::Vector3::Right
					: DirectX::SimpleMath::Vector3::Left;
			}
			else
			{
				pushVectorLocal = (localPosition.z > 0.0f)
					? DirectX::SimpleMath::Vector3::Backward
					: DirectX::SimpleMath::Vector3::Forward;
			}
			distanceSquared = 1.0f;
		}

		// ロボットの半径よりも近い場合に押し出す
		float distance = sqrtf(distanceSquared);

		if (distance < radius)
		{
			// ローカル空間の押し出しベクトルをワールド空間に変換して、Y軸方向は無視する
			DirectX::SimpleMath::Vector3 pushDirectionLocal = pushVectorLocal / distance;
			DirectX::SimpleMath::Vector3 pushDirectionWorld =
				DirectX::SimpleMath::Vector3::Transform(pushDirectionLocal, wall.Orientation);
			pushDirectionWorld.y = 0.0f;

			if (pushDirectionWorld.LengthSquared() > EPSILON)
			{
				// 押し出しベクトルを正規化して、ロボットの座標を押し出す
				pushDirectionWorld.Normalize();
				float pushAmount = radius - distance;
				pos += pushDirectionWorld * pushAmount;

				// 押し出し方向に速度が向いている場合は、速度を押し出し方向の成分だけ減算する
				float dotProduct = velocity.Dot(pushDirectionWorld);
				if (dotProduct < 0.0f)
				{
					velocity -= pushDirectionWorld * dotProduct;
				}
			}
		}
	}
}

/**
 * @brief エリア制限による座標のクランプ処理
 * @param[in,out] pos ロボットの座標
 */
void StageManager::ApplyAreaLimits(DirectX::SimpleMath::Vector3& pos)
{
	if (pos.x < -LIMIT_X_MINUS) { pos.x = -LIMIT_X_MINUS; }
	else if (pos.x > LIMIT_X_PLUS) { pos.x = LIMIT_X_PLUS; }

	if (pos.z < -LIMIT_Z_MINUS) { pos.z = -LIMIT_Z_MINUS; }
	else if (pos.z > LIMIT_Z_PLUS) { pos.z = LIMIT_Z_PLUS; }

	if (pos.y < LIMIT_Y_FLOOR) { pos.y = LIMIT_Y_FLOOR; }
	else if (pos.y > LIMIT_Y_CEILING) { pos.y = LIMIT_Y_CEILING; }
}

/**
 * @brief 疑似円柱（多角柱）を生成して壁リストに追加する
 * @param[in] center 中心座標
 * @param[in] radius 半径
 * @param[in] height 高さ
 * @param[in] segments 分割数
 */
void StageManager::CreatePseudoCylinder(const DirectX::SimpleMath::Vector3& center, 
	float radius, float height, int segments)
{
	// 1つのブロックが担当する角度
	float angleStep = DirectX::XM_2PI / static_cast<float>(segments);

	// OBB(箱)の横幅を計算する（円に外接する多角形の一辺の長さ）
	float segmentWidth = 2.0f * radius * tanf(angleStep * HALF_RATIO) * CYLINDER_WIDTH_MARGIN;
	float segmentDepth = radius;
	float halfHeight = height * HALF_RATIO;

	// 指定された分割数だけOBBを生成し、円を描くように並べる
	for (int i = 0; i < segments; ++i)
	{
		float angle = i * angleStep;
		float offsetRadius = radius * HALF_RATIO;

		float posX = center.x + sinf(angle) * offsetRadius;
		float posZ = center.z + cosf(angle) * offsetRadius;
		float posY = center.y;

		// OBBの中心座標を計算
		DirectX::SimpleMath::Vector3 position(posX, posY, posZ);

		// 箱の向きを円周の接線に沿うように回転させる
		DirectX::SimpleMath::Quaternion rotation = 
			DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::Up, angle);

		// OBBを生成して壁リストに追加
		DirectX::BoundingOrientedBox box;
		box.Center = position;
		box.Extents = DirectX::XMFLOAT3(segmentWidth * HALF_RATIO, halfHeight, segmentDepth * HALF_RATIO);
		box.Orientation = rotation;

		m_walls.push_back(box);
	}
}

/**
 * @brief 地面の高さを取得
 * @param[in] pos 調査する座標
 * @return 地面の高さ(Y)
 */
float StageManager::GetGroundHeight(const DirectX::SimpleMath::Vector3& pos) const
{
	float highestGroundHeight = 0.0f;

	// 指定された座標の少し上から真下へ向けてレイを撃つ
	DirectX::XMVECTOR rayOrigin = DirectX::XMVectorSet(pos.x, pos.y + GROUND_RAY_START_OFFSET_Y, pos.z, 0.0f);
	DirectX::XMVECTOR rayDirection = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

	for (const auto& obb : m_walls)
	{
		float distance = 0.0f;
		if (obb.Intersects(rayOrigin, rayDirection, distance))
		{
			// レイの開始位置からヒット距離を引いて面のY座標を算出する
			float hitHeight = (pos.y + GROUND_RAY_START_OFFSET_Y) - distance;

			// 見つかった地面の中で最も高い場所を採用する
			if (hitHeight > highestGroundHeight && hitHeight <= pos.y + HALF_RATIO)
			{
				highestGroundHeight = hitHeight;
			}
		}
	}

	return highestGroundHeight;
}