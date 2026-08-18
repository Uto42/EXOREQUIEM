/*****************************************************************//**
 * @file    ParticleManager.cpp
 * @brief   パーティクル群のメモリ管理、更新、およびジオメトリシェーダを用いた一括描画の実装
 *
 * @author  甲斐勇翔
 * @date    2026/07/13
 *********************************************************************/

#include "pch.h"
#include "Game/Systems/Effect/Particle/ParticleManager.h"
#include <fstream>
#include <WICTextureLoader.h>
#include <d3dcompiler.h>

// シェーダーファイルのパス定義
static const wchar_t* SHADER_FILE_VS = L"Resources/Shaders/ParticleVS.cso";
static const wchar_t* SHADER_FILE_GS = L"Resources/Shaders/ParticleGS.cso";
static const wchar_t* SHADER_FILE_PS = L"Resources/Shaders/ParticlePS.cso";

/**
 * @brief バイナリ読み込みヘルパー
 * @param[in] fileName ファイルパス
 * @return std::vector<char> ファイルデータ
 */
static std::vector<char> LoadShaderFile(const wchar_t* fileName)
{
    std::ifstream ifs(fileName, std::ios::in | std::ios::binary);

    if (!ifs)
    {
        return {};
    }

    ifs.seekg(0, std::ios::end);
    size_t size = (size_t)ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<char> data(size);
    ifs.read(data.data(), size);
    return data;
}

/**
 * @brief コンストラクタ
 */
ParticleManager::ParticleManager()
{
}

/**
 * @brief デストラクタ
 */
ParticleManager::~ParticleManager()
{
}

/**
 * @brief 初期化処理
 * @param[in] device  ID3D11Device
 * @param[in] context ID3D11DeviceContext
 */
void ParticleManager::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
    m_device = device;
    m_context = context;
    m_states = std::make_unique<DirectX::CommonStates>(device);

    // シェーダ読み込み
    auto vsData = LoadShaderFile(SHADER_FILE_VS);
    auto gsData = LoadShaderFile(SHADER_FILE_GS);
    auto psData = LoadShaderFile(SHADER_FILE_PS);

    if (vsData.empty() || gsData.empty() || psData.empty())
    {
        OutputDebugStringA("ERROR: Failed to load particle shaders.\n");
        return;
    }

    device->CreateVertexShader(vsData.data(), vsData.size(), nullptr, m_vertexShader.ReleaseAndGetAddressOf());
    device->CreateGeometryShader(gsData.data(), gsData.size(), nullptr, m_geometryShader.ReleaseAndGetAddressOf());
    device->CreatePixelShader(psData.data(), psData.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf());

    // 入力レイアウト作成
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        vsData.data(),
        vsData.size(),
        m_inputLayout.ReleaseAndGetAddressOf()
    );

    // 定数バッファ作成
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(cbPerFrame);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.ReleaseAndGetAddressOf());

    // 頂点バッファ作成
    // 最大数分だけ確保しておく
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(VertexParticle) * MAX_PARTICLES;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&vbDesc, nullptr, m_vertexBuffer.ReleaseAndGetAddressOf());
}

/**
 * @brief テクスチャ読み込み
 * @param[in] path ファイルパス
 */
void ParticleManager::LoadTexture(const wchar_t* path)
{
    if (m_device)
    {
        DirectX::CreateWICTextureFromFile(m_device, path, nullptr, m_texture.ReleaseAndGetAddressOf());
    }
}

/**
 * @brief パーティクル追加
 */
void ParticleManager::SpawnParticle(float life, DirectX::SimpleMath::Vector3 pos, 
    DirectX::SimpleMath::Vector3 velocity, DirectX::SimpleMath::Vector3 accele, 
    DirectX::SimpleMath::Vector3 startScale, DirectX::SimpleMath::Vector3 endScale, 
    DirectX::SimpleMath::Color startColor, DirectX::SimpleMath::Color endColor)
{
    // メモリ確保とリストへの追加を行う
    m_particles.push_back(std::make_unique<Particle>(
        life, pos, velocity, accele, startScale, endScale, startColor, endColor));
}

/**
 * @brief 更新処理
 * @param[in] dt デルタタイム
 */
void ParticleManager::Update(float dt)
{
    // パーティクルの更新と削除
    for (auto it = m_particles.begin(); it != m_particles.end(); )
    {
        // Updateが false を返したら寿命切れ
        if (!(*it)->Update(dt))
        {
            it = m_particles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

/**
 * @brief 描画処理
 * @param[in] context デバイスコンテキスト
 * @param[in] view ビュー行列
 * @param[in] proj 射影行列
 */
void ParticleManager::Render(ID3D11DeviceContext* context,
    const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
    if (m_particles.empty())
    {
        return;
    }

    // 描画用頂点データの作成
    m_vertexList.clear();

    // vectorのメモリ再確保を防ぐため、事前にreserveする
    m_vertexList.reserve(m_particles.size());

    for (const auto& p : m_particles)
    {
        VertexParticle v;
        v.pos = p->GetPosition();
        v.color = p->GetNowColor();
        // Xに現在のサイズを入れる (GSでビルボード展開に使用)
        v.size = DirectX::SimpleMath::Vector2(p->GetNowScale().x, p->GetNowScale().y);

        m_vertexList.push_back(v);

        // バッファサイズを超えたら打ち止め
        if (m_vertexList.size() >= MAX_PARTICLES)
        {
            break;
        }
    }

    // 頂点バッファの更新 (Map / Unmap)
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, m_vertexList.data(), sizeof(VertexParticle) * m_vertexList.size());
        context->Unmap(m_vertexBuffer.Get(), 0);
    }

    // 定数バッファ更新
    cbPerFrame cb;
    cb.matWorld = DirectX::SimpleMath::Matrix::Identity; // ワールド行列は単位行列（個別の位置は頂点データにある）
    cb.matView = view.Transpose();
    cb.matProj = proj.Transpose();
    cb.diffuse = DirectX::SimpleMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 全体の色乗算（白）

    context->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // パイプライン設定

    // 加算合成
    context->OMSetBlendState(m_states->Additive(), nullptr, 0xFFFFFFFF);

    // 深度書き込みなし
    context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    // カリングなし
    context->RSSetState(m_states->CullNone());

    context->IASetInputLayout(m_inputLayout.Get());

    // ジオメトリシェーダーで展開するため、入力は「点 (POINTLIST)」
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    UINT stride = sizeof(VertexParticle);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->GSSetShader(m_geometryShader.Get(), nullptr, 0);
    context->GSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());

    // 描画
    context->Draw((UINT)m_vertexList.size(), 0);

    // 後始末
    context->GSSetShader(nullptr, nullptr, 0);

    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
}