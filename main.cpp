﻿#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3dcompiler.h>
#define DIRECTINPUT_VERSION     0x0800   // DirectInputのバージョン指定
#include <dinput.h>
#include <wrl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

// 頂点データ構造体
struct Vertex
{
    XMFLOAT3 pos;       // xyz座標
    XMFLOAT3 normal;    // 法線ベクトル
    XMFLOAT2 uv;        // uv座標
};

// 定数バッファ用データ構造体（マテリアル）
struct ConstBufferDataMaterial {
    XMFLOAT4 color; // 色 (RGBA)
};

// 定数バッファ用データ構造体（３Ｄ変換行列）
struct ConstBufferDataTransform {
    XMMATRIX mat;   // ３Ｄ変換行列
};

// 3Dオブジェクト型
struct Object3d
{
    // 定数バッファ（行列用）
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffTransform;
    // 定数バッファマップ（行列用）
    ConstBufferDataTransform* constMapTransform;
    // アフィン変換情報
    XMFLOAT3 scale = { 1,1,1 };
    XMFLOAT3 rotation = { 0,0,0 };
    XMFLOAT3 position = { 0,0,0 };
    // ワールド変換行列
    XMMATRIX matWorld;
    // 親オブジェクトへのポインタ
    Object3d* parent = nullptr;
};

// 3Dオブジェクト初期化
void InitializeObject3d(Object3d* object, ID3D12Device* device) {
    HRESULT result;

    // 定数バッファのヒープ設定
    D3D12_HEAP_PROPERTIES heapProp{};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    // 定数バッファのリソース設定
    D3D12_RESOURCE_DESC resdesc{};
    resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resdesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff;
    resdesc.Height = 1;
    resdesc.DepthOrArraySize = 1;
    resdesc.MipLevels = 1;
    resdesc.SampleDesc.Count = 1;
    resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // 定数バッファの生成
    result = device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resdesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&object->constBuffTransform));
    assert(SUCCEEDED(result));

    // 定数バッファのマッピング
    result = object->constBuffTransform->Map(0, nullptr, (void**)&object->constMapTransform);
    assert(SUCCEEDED(result));
}

void UpdateObject3d(Object3d* object, XMMATRIX& matView, XMMATRIX& matProjection) {
    XMMATRIX matScale, matRot, matTrans;

    // スケール、回転、平行移動行列の計算
    matScale = XMMatrixScaling(object->scale.x, object->scale.y, object->scale.z);
    matRot = XMMatrixIdentity();
    matRot *= XMMatrixRotationZ(object->rotation.z);
    matRot *= XMMatrixRotationX(object->rotation.x);
    matRot *= XMMatrixRotationY(object->rotation.y);
    matTrans = XMMatrixTranslation(object->position.x, object->position.y, object->position.z);

    // ワールド行列の合成
    object->matWorld = XMMatrixIdentity(); // 変形をリセット
    object->matWorld *= matScale; // ワールド行列にスケーリングを反映
    object->matWorld *= matRot; // ワールド行列に回転を反映
    object->matWorld *= matTrans; // ワールド行列に平行移動を反映

    // 親オブジェクトがあれば
    if (object->parent != nullptr) {
        // 親オブジェクトのワールド行列を掛ける
        object->matWorld *= object->parent->matWorld;
    }

    // 定数バッファへデータ転送
    object->constMapTransform->mat = object->matWorld * matView * matProjection;
}

void DrawObject3d(Object3d* object, ID3D12GraphicsCommandList* commandList, D3D12_VERTEX_BUFFER_VIEW& vbView, D3D12_INDEX_BUFFER_VIEW& ibView, UINT numIndices) {
    // 頂点バッファの設定
    commandList->IASetVertexBuffers(0, 1, &vbView);
    // インデックスバッファの設定
    commandList->IASetIndexBuffer(&ibView);
    // 定数バッファビュー(CBV)の設定コマンド
    commandList->SetGraphicsRootConstantBufferView(2, object->constBuffTransform->GetGPUVirtualAddress());

    // 描画コマンド
    commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
}

ScratchImage LoadImageFromFile(const wchar_t* path) {
    TexMetadata metadata{};
    ScratchImage scratchImg{};
    // WICテクスチャのロード
    HRESULT result = LoadFromWICFile(
        path,
        WIC_FLAGS_NONE,
        &metadata, scratchImg);

    ScratchImage mipChain{};
    // ミップマップ生成
    result = GenerateMipMaps(
        scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
        TEX_FILTER_DEFAULT, 0, mipChain);
    if (SUCCEEDED(result)) {
        scratchImg = std::move(mipChain);
        metadata = scratchImg.GetMetadata();
    }

    return scratchImg;
}

ComPtr<ID3D12Resource> CreateTexture(ID3D12Device* device, const TexMetadata& metadata) {
    // ヒープ設定
    D3D12_HEAP_PROPERTIES textureHeapProp{};
    textureHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    textureHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    textureHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    // リソース設定
    D3D12_RESOURCE_DESC textureResourceDesc{};
    textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureResourceDesc.Format = metadata.format;
    textureResourceDesc.Width = metadata.width;
    textureResourceDesc.Height = (UINT)metadata.height;
    textureResourceDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
    textureResourceDesc.MipLevels = (UINT16)metadata.mipLevels;
    textureResourceDesc.SampleDesc.Count = 1;
    // テクスチャバッファの生成
    ComPtr<ID3D12Resource> texBuff;
    HRESULT result = device->CreateCommittedResource(
        &textureHeapProp,
        D3D12_HEAP_FLAG_NONE,
        &textureResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // テクスチャ用指定
        nullptr,
        IID_PPV_ARGS(&texBuff));
    if (result == S_OK) {
        return texBuff;
    }
    return nullptr;
}

void UploadSubresources(ID3D12Resource* texBuff, const ScratchImage& scratchImg) {
    // 全ミップマップについて
    size_t mipLevels = scratchImg.GetMetadata().mipLevels;
    for (size_t i = 0; i < mipLevels; i++) {
        // ミップマップレベルを指定してイメージを取得
        const Image* img = scratchImg.GetImage(i, 0, 0);
        // テクスチャバッファにデータ転送
        HRESULT result = texBuff->WriteToSubresource(
            (UINT)i,
            nullptr,              // 全領域へコピー
            img->pixels,          // 元データアドレス
            (UINT)img->rowPitch,  // 1ラインサイズ
            (UINT)img->slicePitch // 1枚サイズ
        );
        assert(SUCCEEDED(result));
    }
}

// ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    // メッセージ応じてゲーム固有の処理を行う
    switch (msg) {
        // ウィンドウが破棄された
    case WM_DESTROY:
        // OSに対して、アプリの終了を伝える
        PostQuitMessage(0);
        return 0;
    }

    // 標準のメッセージ処理を行う
    return DefWindowProc(hwnd, msg, wparam, lparam);
}


// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

#pragma region WindowsAPI初期化処理
    // ウィンドウサイズ
    const int window_width = 1280;  // 横幅
    const int window_height = 720;  // 縦幅

    // ウィンドウクラスの設定
    WNDCLASSEX w{};
    w.cbSize = sizeof(WNDCLASSEX);
    w.lpfnWndProc = (WNDPROC)WindowProc; // ウィンドウプロシージャを設定
    w.lpszClassName = L"DirectXGame"; // ウィンドウクラス名
    w.hInstance = GetModuleHandle(nullptr); // ウィンドウハンドル
    w.hCursor = LoadCursor(NULL, IDC_ARROW); // カーソル指定

    // ウィンドウクラスをOSに登録する
    RegisterClassEx(&w);
    // ウィンドウサイズ{ X座標 Y座標 横幅 縦幅 }
    RECT wrc = { 0, 0, window_width, window_height };
    // 自動でサイズを補正する
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    // ウィンドウオブジェクトの生成
    HWND hwnd = CreateWindow(w.lpszClassName, // クラス名
        L"DirectXGame",         // タイトルバーの文字
        WS_OVERLAPPEDWINDOW,        // 標準的なウィンドウスタイル
        CW_USEDEFAULT,              // 表示X座標（OSに任せる）
        CW_USEDEFAULT,              // 表示Y座標（OSに任せる）
        wrc.right - wrc.left,       // ウィンドウ横幅
        wrc.bottom - wrc.top,   // ウィンドウ縦幅
        nullptr,                // 親ウィンドウハンドル
        nullptr,                // メニューハンドル
        w.hInstance,            // 呼び出しアプリケーションハンドル
        nullptr);               // オプション

    // ウィンドウを表示状態にする
    ShowWindow(hwnd, SW_SHOW);

    MSG msg{};  // メッセージ
#pragma endregion

#pragma region DirectX初期化処理
    // DirectX初期化処理　ここから
    HRESULT result;
    ComPtr<ID3D12Device> device;
    ComPtr<IDXGIFactory7> dxgiFactory;
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;

#ifdef _DEBUG
    //デバッグレイヤーをオンに
    ComPtr<ID3D12Debug1> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(TRUE);
    }
#endif

    // DXGIファクトリーの生成
    result = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
    assert(SUCCEEDED(result));

    // アダプターの列挙用
    std::vector<ComPtr<IDXGIAdapter4>> adapters;
    // ここに特定の名前を持つアダプターオブジェクトが入る
    ComPtr<IDXGIAdapter4> tmpAdapter;

    // パフォーマンスが高いものから順に、全てのアダプターを列挙する
    for (UINT i = 0;
        dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND;
        i++) {
        // 動的配列に追加する
        adapters.push_back(tmpAdapter);
    }

    // 妥当なアダプタを選別する
    for (size_t i = 0; i < adapters.size(); i++) {
        DXGI_ADAPTER_DESC3 adapterDesc;
        // アダプターの情報を取得する
        adapters[i]->GetDesc3(&adapterDesc);

        // ソフトウェアデバイスを回避
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            // デバイスを採用してループを抜ける
            tmpAdapter = adapters[i];
            break;
        }
    }

    // 対応レベルの配列
    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL featureLevel;

    for (size_t i = 0; i < _countof(levels); i++) {
        // 採用したアダプターでデバイスを生成
        result = D3D12CreateDevice(tmpAdapter.Get(), levels[i], IID_PPV_ARGS(&device));
        if (result == S_OK) {
            // デバイスを生成できた時点でループを抜ける
            featureLevel = levels[i];
            break;
        }
    }

#ifdef _DEBUG
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        // 抑制するエラー
        D3D12_MESSAGE_ID denyIds[] = {
            /*
             * Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
             * https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
             */
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };
        // 抑制する表示レベル
        D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;
        // 指定したエラーの表示を抑制する
        infoQueue->PushStorageFilter(&filter);
        // エラー時にブレークを発生させる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    }
#endif

    // コマンドアロケータを生成
    result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    assert(SUCCEEDED(result));

    // コマンドリストを生成
    result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
    assert(SUCCEEDED(result));

    //コマンドキューの設定
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    //コマンドキューを生成
    result = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
    assert(SUCCEEDED(result));

    // スワップチェーンの設定
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = 1280;
    swapChainDesc.Height = 720;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 色情報の書式
    swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
    swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // バックバッファ用
    swapChainDesc.BufferCount = 2;  // バッファ数を２つに設定
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は破棄
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapChain1;
    // スワップチェーンの生成
    result = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
    assert(SUCCEEDED(result));

    // SwapChain4を得る
    swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain));
    assert(SUCCEEDED(result));

    // デスクリプタヒープの設定
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー
    rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount;    // 裏表の２つ
    // デスクリプタヒープの生成
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    // バックバッファ
    std::vector<ComPtr<ID3D12Resource>> backBuffers;
    backBuffers.resize(swapChainDesc.BufferCount);

    // スワップチェーンの全てのバッファについて処理する
    for (size_t i = 0; i < backBuffers.size(); i++) {
        // スワップチェーンからバッファを取得
        swapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
        // デスクリプタヒープのハンドルを取得
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        // 裏か表かでアドレスがずれる
        rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
        // レンダーターゲットビューの設定
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
        // シェーダーの計算結果をSRGBに変換して書き込む
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        // レンダーターゲットビューの生成
        device->CreateRenderTargetView(backBuffers[i].Get(), &rtvDesc, rtvHandle);
    }

    // リソース設定
    D3D12_RESOURCE_DESC depthResourceDesc{};
    depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthResourceDesc.Width = window_width; // レンダーターゲットに合わせる
    depthResourceDesc.Height = window_height; // レンダーターゲットに合わせる
    depthResourceDesc.DepthOrArraySize = 1;
    depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット
    depthResourceDesc.SampleDesc.Count = 1;
    depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // デプスステンシル

    // 深度値用ヒーププロパティ
    D3D12_HEAP_PROPERTIES depthHeapProp{};
    depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    // 深度値のクリア設定
    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f; // 深度値1.0f（最大値）でクリア
    depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット
    // リソース生成
    ComPtr<ID3D12Resource> depthBuff;
    result = device->CreateCommittedResource(
        &depthHeapProp,
        D3D12_HEAP_FLAG_NONE,
        &depthResourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値書き込みに使用
        &depthClearValue,
        IID_PPV_ARGS(&depthBuff));

    // 深度ビュー用デスクリプタヒープ作成
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1つ
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // デプスステンシルビュー
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

    // 深度ビュー作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(
        depthBuff.Get(),
        &dsvDesc,
        dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // フェンスの生成
    ComPtr<ID3D12Fence> fence;
    UINT64 fenceVal = 0;

    result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    assert(SUCCEEDED(result));

    // DirectX初期化処理　ここまで
#pragma endregion

    // DirectInputの初期化
    ComPtr<IDirectInput8> directInput;
    result = DirectInput8Create(
        w.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
    assert(SUCCEEDED(result));

    // キーボードデバイスの生成
    ComPtr<IDirectInputDevice8> keyboard;
    result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    // 入力データ形式のセット
    result = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
    assert(SUCCEEDED(result));
    // 排他制御レベルのセット
    result = keyboard->SetCooperativeLevel(
        hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(result));

#pragma region 描画初期化処理

    // 頂点データ
    Vertex vertices[] = {
        //  x      y      z       u     v
        // 前
        {{ -5.0f, -5.0f, -5.0f}, {},  {0.0f, 1.0f}}, // 左下
        {{ -5.0f,  5.0f, -5.0f}, {},  {0.0f, 0.0f}}, // 左上
        {{  5.0f, -5.0f, -5.0f}, {},  {1.0f, 1.0f}}, // 右下
        {{  5.0f,  5.0f, -5.0f}, {},  {1.0f, 0.0f}}, // 右上
        // 後(前面とZ座標の符号が逆)
        {{  5.0f, -5.0f,  5.0f}, {},  {1.0f, 1.0f}}, // 右下
        {{  5.0f,  5.0f,  5.0f}, {},  {1.0f, 0.0f}}, // 右上
        {{ -5.0f, -5.0f,  5.0f}, {},  {0.0f, 1.0f}}, // 左下
        {{ -5.0f,  5.0f,  5.0f}, {},  {0.0f, 0.0f}}, // 左上
        // 左
        {{-5.0f, -5.0f, +5.0f}, {},  {0.0f, 1.0f}}, // 左下
        {{-5.0f, +5.0f, +5.0f}, {},  {0.0f, 0.0f}}, // 左上
        {{-5.0f, -5.0f, -5.0f}, {},  {1.0f, 1.0f}}, // 右下
        {{-5.0f, +5.0f, -5.0f}, {},  {1.0f, 0.0f}}, // 右上
        // 右（左面とX座標の符号
        {{+5.0f, -5.0f, -5.0f}, {},  {0.0f, 1.0f}}, // 左下
        {{+5.0f, +5.0f, -5.0f}, {},  {0.0f, 0.0f}}, // 左上
        {{+5.0f, -5.0f, +5.0f}, {},  {1.0f, 1.0f}}, // 右下
        {{+5.0f, +5.0f, +5.0f}, {},  {1.0f, 0.0f}}, // 右上
        // 下
        {{+5.0f, -5.0f, -5.0f}, {},  {0.0f, 1.0f}}, // 左下
        {{+5.0f, -5.0f, +5.0f}, {},  {0.0f, 0.0f}}, // 左上
        {{-5.0f, -5.0f, -5.0f}, {},  {1.0f, 1.0f}}, // 右下
        {{-5.0f, -5.0f, +5.0f}, {},  {1.0f, 0.0f}}, // 右上
        // 上（下面とY座標の符号
        {{-5.0f, +5.0f, -5.0f}, {},  {0.0f, 1.0f}}, // 左下
        {{-5.0f, +5.0f, +5.0f}, {},  {0.0f, 0.0f}}, // 左上
        {{+5.0f, +5.0f, -5.0f}, {},  {1.0f, 1.0f}}, // 右下
        {{+5.0f, +5.0f, +5.0f}, {},  {1.0f, 0.0f}}, // 右上
    };

    // インデックスデータ
    uint16_t indices[] =
    {
        0,  1,  3,  3,  2,  0,
        4,  5,  7,  7,  6,  4,
        8,  9,  11, 11, 10, 8,
        12, 13, 15, 15, 14, 12,
        16, 17, 19, 19, 18, 16,
        20, 21, 23, 23, 22, 20
    };

    // 法線の計算
    for (int i = 0; i < _countof(indices) / 3; i++)
    {// 三角形１つごとに計算していく
        // 三角形のインデックスを取り出して、一時的な変数に入れる
        unsigned short index0 = indices[i * 3 + 0];
        unsigned short index1 = indices[i * 3 + 1];
        unsigned short index2 = indices[i * 3 + 2];
        // 三角形を構成する頂点座標をベクトルに代入
        XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
        XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
        XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
        // p0→p1ベクトル、p0→p2ベクトルを計算　（ベクトルの減算）
        XMVECTOR v1 = XMVectorSubtract(p1, p0);
        XMVECTOR v2 = XMVectorSubtract(p2, p0);
        // 外積は両方から垂直なベクトル
        XMVECTOR normal = XMVector3Cross(v1, v2);
        // 正規化（長さを1にする)
        normal = XMVector3Normalize(normal);
        // 求めた法線を頂点データに代入
        XMStoreFloat3(&vertices[index0].normal, normal);
        XMStoreFloat3(&vertices[index1].normal, normal);
        XMStoreFloat3(&vertices[index2].normal, normal);
    }

    // 頂点データ全体のサイズ = 頂点データ一つ分のサイズ * 頂点データの要素数
    UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));

    // 頂点バッファの設定
    D3D12_HEAP_PROPERTIES heapProp{};   // ヒープ設定
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // GPUへの転送用
    // リソース設定
    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeVB; // 頂点データ全体のサイズ
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // 頂点バッファの生成
    ComPtr<ID3D12Resource> vertBuff;
    result = device->CreateCommittedResource(
        &heapProp, // ヒープ設定
        D3D12_HEAP_FLAG_NONE,
        &resDesc, // リソース設定
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertBuff));
    assert(SUCCEEDED(result));

    // インデックスデータ全体のサイズ
    UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(indices));

    // リソース設定
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeIB; // インデックス情報が入る分のサイズ
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    // インデックスバッファの生成
    ComPtr<ID3D12Resource> indexBuff;
    result = device->CreateCommittedResource(
        &heapProp, // ヒープ設定
        D3D12_HEAP_FLAG_NONE,
        &resDesc, // リソース設定
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexBuff));

    // GPU上のバッファに対応した仮想メモリ(メインメモリ上)を取得
    Vertex* vertMap = nullptr;
    result = vertBuff->Map(0, nullptr, (void**)&vertMap);
    assert(SUCCEEDED(result));
    // 全頂点に対して
    for (int i = 0; i < _countof(vertices); i++) {
        vertMap[i] = vertices[i];   // 座標をコピー
    }
    // 繋がりを解除
    vertBuff->Unmap(0, nullptr);

    // インデックスバッファをマッピング
    uint16_t* indexMap = nullptr;
    result = indexBuff->Map(0, nullptr, (void**)&indexMap);
    // 全インデックスに対して
    for (int i = 0; i < _countof(indices); i++)
    {
        indexMap[i] = indices[i];   // インデックスをコピー
    }
    // マッピング解除
    indexBuff->Unmap(0, nullptr);

    // 頂点バッファビューの作成
    D3D12_VERTEX_BUFFER_VIEW vbView{};
    // GPU仮想アドレス
    vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
    // 頂点バッファのサイズ
    vbView.SizeInBytes = sizeVB;
    // 頂点１つ分のデータサイズ
    vbView.StrideInBytes = sizeof(vertices[0]);

    // インデックスバッファビューの作成
    D3D12_INDEX_BUFFER_VIEW ibView{};
    ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes = sizeIB;

    ID3DBlob* vsBlob = nullptr; // 頂点シェーダオブジェクト
    ID3DBlob* psBlob = nullptr; // ピクセルシェーダオブジェクト
    ID3DBlob* errorBlob = nullptr; // エラーオブジェクト

    // 頂点シェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/BasicVS.hlsl",  // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "vs_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &vsBlob, &errorBlob);
    // エラーなら
    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string error;
        error.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            error.begin());
        error += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(error.c_str());
        assert(0);
    }

    // ピクセルシェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/BasicPS.hlsl",   // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "ps_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &psBlob, &errorBlob);
    // エラーなら
    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string error;
        error.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            error.begin());
        error += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(error.c_str());
        assert(0);
    }

    // 頂点レイアウト
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { // xyz座標(1行で書いたほうが見やすい)
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        { // 法線ベクトル(1行で書いたほうが見やすい)
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        { // uv座標(1行で書いたほうが見やすい)
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
    };

    // グラフィックスパイプライン設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
    // シェーダーの設定
    pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
    pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
    pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();
    // サンプルマスクの設定
    pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定
    // ラスタライザの設定
    pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // カリングしない
    pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;  // 背面をカリング
    pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ポリゴン内塗りつぶし
    //pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME; // ワイヤーフレーム
    pipelineDesc.RasterizerState.DepthClipEnable = true; // 深度クリッピングを有効に
    // ブレンドステート
    pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;  // RBGA全てのチャンネルを描画
    // pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    // レンダーターゲットのブレンド設定
    D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // RBGA全てのチャンネルを描画
    blenddesc.BlendEnable = true;                   // ブレンドを有効にする
    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;    // 加算
    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;      // ソースの値を100% 使う
    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;    // デストの値を  0% 使う

    // 半透明合成
    blenddesc.BlendOp = D3D12_BLEND_OP_ADD;             // 加算
    blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;         // ソースのアルファ値
    blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;    // 1.0f-ソースのアルファ値

    // デプスステンシルステートの設定
    pipelineDesc.DepthStencilState.DepthEnable = true; // 深度テストを行う
    pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 書き込み許可
    pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 小さければ合格
    pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット

    // 頂点レイアウトの設定
    pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
    pipelineDesc.InputLayout.NumElements = _countof(inputLayout);
    // 図形の形状設定
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // その他の設定
    pipelineDesc.NumRenderTargets = 1; // 描画対象は1つ
    pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0～255指定のRGBA
    pipelineDesc.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング

    // デスクリプタレンジの設定
    D3D12_DESCRIPTOR_RANGE descriptorRange{};
    descriptorRange.NumDescriptors = 1;         //一度の描画に使うテクスチャが1枚なので1
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.BaseShaderRegister = 0;     //テクスチャレジスタ0番
    descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    // ルートパラメータの設定
    D3D12_ROOT_PARAMETER rootParams[3] = {};
    // 定数バッファ0番
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;   // 種類
    rootParams[0].Descriptor.ShaderRegister = 0;                   // 定数バッファ番号
    rootParams[0].Descriptor.RegisterSpace = 0;                    // デフォルト値
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;  // 全てのシェーダから見える
    // テクスチャレジスタ0番
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;   //種類
    rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;              //デスクリプタレンジ数
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;       //全てのシェーダから見える
    // 定数バッファ1番
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;   // 種類
    rootParams[2].Descriptor.ShaderRegister = 1;                   // 定数バッファ番号
    rootParams[2].Descriptor.RegisterSpace = 0;                    // デフォルト値
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;  // 全てのシェーダから見える

    // テクスチャサンプラーの設定
    D3D12_STATIC_SAMPLER_DESC samplerDesc{};
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 //横繰り返し（タイリング）
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 //縦繰り返し（タイリング）
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;                 //奥行繰り返し（タイリング）
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;  //ボーダーの時は黒
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;                   //全てリニア補間
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;                                 //ミップマップ最大値
    samplerDesc.MinLOD = 0.0f;                                              //ミップマップ最小値
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;           //ピクセルシェーダからのみ使用可能

    // ルートシグネチャ
    ComPtr<ID3D12RootSignature> rootSignature;
    // ルートシグネチャの設定
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParams; //ルートパラメータの先頭アドレス
    rootSignatureDesc.NumParameters = _countof(rootParams);        //ルートパラメータ数
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.NumStaticSamplers = 1;
    // ルートシグネチャのシリアライズ
    ID3DBlob* rootSigBlob = nullptr;
    result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
    assert(SUCCEEDED(result));
    result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(result));
    rootSigBlob->Release();
    // パイプラインにルートシグネチャをセット
    pipelineDesc.pRootSignature = rootSignature.Get();
    // パイプランステートの生成
    ComPtr<ID3D12PipelineState> pipelineState;
    result = device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
    assert(SUCCEEDED(result));

#pragma endregion

    // 3Dオブジェクトの数
    const size_t kObjectCount = 50;
    // 3Dオブジェクトの配列
    Object3d object3ds[kObjectCount];

    srand(1);

    // 配列内の全オブジェクトに対して
    for (int i = 0; i < _countof(object3ds); i++) {
        // 初期化
        InitializeObject3d(&object3ds[i], device.Get());

        // ここから↓は親子構造のサンプル
        // 先頭以外なら
        if (i > 0) {
            // 一つ前のオブジェクトを親オブジェクトとする
            //object3ds[i].parent = &object3ds[i - 1];
            // 親オブジェクトの9割の大きさ
            object3ds[i].scale = { 0.9f,0.9f,0.9f };
            // 親オブジェクトに対してZ軸まわりに30度回転
            object3ds[i].rotation = { 0.0f, 0.0f, XMConvertToRadians(30.0f) };
            // ランダムな座標設定
            float posX = (float)rand() / RAND_MAX * 200 - 100;
            float posY = (float)rand() / RAND_MAX * 200 - 100;
            float posZ = (float)rand() / RAND_MAX * 50 + 50;
            object3ds[i].position = { posX,posY,posZ };
        }
    }

    // ヒープ設定
    D3D12_HEAP_PROPERTIES cbHeapProp{};
    cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;                   // GPUへの転送用
    // リソース設定
    D3D12_RESOURCE_DESC cbResourceDesc{};
    cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbResourceDesc.Width = (sizeof(ConstBufferDataMaterial) + 0xff) & ~0xff;   // 256バイトアラインメント
    cbResourceDesc.Height = 1;
    cbResourceDesc.DepthOrArraySize = 1;
    cbResourceDesc.MipLevels = 1;
    cbResourceDesc.SampleDesc.Count = 1;
    cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> constBuffMaterial;
    // 定数バッファの生成
    result = device->CreateCommittedResource(
        &cbHeapProp, // ヒープ設定
        D3D12_HEAP_FLAG_NONE,
        &cbResourceDesc, // リソース設定
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constBuffMaterial));
    assert(SUCCEEDED(result));
    // 定数バッファのマッピング
    ConstBufferDataMaterial* constMapMaterial = nullptr;
    result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial); // マッピング
    assert(SUCCEEDED(result));
    // 値を書き込むと自動的に転送される
    constMapMaterial->color = XMFLOAT4(1, 1, 1, 1);

    //// 並行投影行列の計算
    //constMapTransform->mat = XMMatrixOrthographicOffCenterLH(
    //    0.0f, window_width,
    //    window_height, 0.0f,
    //    0.0f, 1.0f);

    // 射影変換行列(透視投影)
    XMMATRIX matProjection = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.0f),
        (float)window_width / window_height,
        0.1f, 1000.0f
    );

    // ビュー変換行列
    XMMATRIX matView;
    XMFLOAT3 eye(0, 0, -100);   // 視点座標
    XMFLOAT3 target(0, 0, 0);   // 注視点座標
    XMFLOAT3 up(0, 1, 0);       // 上方向ベクトル
    matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

    float angle = 0.0f; // カメラの回転角

    // 1枚目のテクスチャ
    ScratchImage scratchImg1 = LoadImageFromFile(L"Resources/mario.jpg");

    // 読み込んだディフューズテクスチャをSRGBとして扱う
    TexMetadata metadata1 = scratchImg1.GetMetadata();
    metadata1.format = MakeSRGB(metadata1.format);

    ComPtr<ID3D12Resource> texBuff1 = CreateTexture(device.Get(), metadata1);
    UploadSubresources(texBuff1.Get(), scratchImg1);

    // 2枚目のテクスチャ
    ScratchImage scratchImg2 = LoadImageFromFile(L"Resources/reimu.png");

    // 読み込んだディフューズテクスチャをSRGBとして扱う
    TexMetadata metadata2 = scratchImg2.GetMetadata();
    metadata2.format = MakeSRGB(metadata2.format);

    ComPtr<ID3D12Resource> texBuff2 = CreateTexture(device.Get(), metadata2);
    UploadSubresources(texBuff2.Get(), scratchImg2);

    // SRVの最大個数
    const size_t kMaxSRVCount = 2056;
    // デスクリプタヒープの設定
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
    srvHeapDesc.NumDescriptors = kMaxSRVCount;
    // 設定を元にSRV用デスクリプタヒープを生成
    ComPtr<ID3D12DescriptorHeap> srvHeap;
    result = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
    assert(SUCCEEDED(result));

    //SRVヒープの先頭ハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

    // シェーダリソースビュー設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // 設定構造体
    srvDesc.Format = DXGI_FORMAT_UNKNOWN; // テクスチャのフォーマットに準拠する
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
    srvDesc.Texture2D.MipLevels = UINT(metadata1.mipLevels);

    // ハンドルの指す位置にシェーダーリソースビュー作成
    device->CreateShaderResourceView(texBuff1.Get(), &srvDesc, srvHandle);

    // 2枚目のシェーダーリソースビューを作成。1枚目のディスクリプタハンドルの次の場所に配置する
    size_t incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    srvHandle.ptr += incrementSize;
    srvDesc.Texture2D.MipLevels = UINT(metadata2.mipLevels);
    device->CreateShaderResourceView(texBuff2.Get(), &srvDesc, srvHandle);

    size_t textureIndex = 0;
    BYTE key[256] = {};

    // ゲームループ
    while (true) {
        // メッセージがある？
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); // キー入力メッセージの処理
            DispatchMessage(&msg); // プロシージャにメッセージを送る
        }

        // ✖ボタンで終了メッセージが来たらゲームループを抜ける
        if (msg.message == WM_QUIT) {
            break;
        }

        // キーボード情報の取得開始
        keyboard->Acquire();
        // 全キーの入力状態を取得する
        keyboard->GetDeviceState(sizeof(key), key);

        //// 数字の0キーが押されていたら
        //if (key[DIK_0]) 
        //{
        //    OutputDebugStringA("Hit 0\n");  // 出力ウィンドウに「Hit 0」と表示
        //}

        // DirectX毎フレーム処理　ここから
        //static float red = 1.0f;

        //if (key[DIK_SPACE]) {
        //    red -= 0.01f;
        //    red = max(0, red);
        //    constMapMaterial->color = XMFLOAT4(red, 1.0f - red, 0, 0.5f);              // RGBAで半透明の赤
        //}

        if (key[DIK_D] || key[DIK_A])
        {
            if (key[DIK_D]) { angle += XMConvertToRadians(1.0f); }
            else if (key[DIK_A]) { angle -= XMConvertToRadians(1.0f); }

            // angleラジアンだけY軸まわりに回転。半径は-100
            eye.x = -100 * sinf(angle);
            eye.z = -100 * cosf(angle);

            matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
        }

        // 座標操作
        if (key[DIK_UP] || key[DIK_DOWN] || key[DIK_RIGHT] || key[DIK_LEFT])
        {
            if (key[DIK_UP]) { object3ds[0].position.y += 1.0f; }
            else if (key[DIK_DOWN]) { object3ds[0].position.y -= 1.0f; }
            if (key[DIK_RIGHT]) { object3ds[0].position.x += 1.0f; }
            else if (key[DIK_LEFT]) { object3ds[0].position.x -= 1.0f; }
        }

        // 全オブジェクトについて処理
        for (size_t i = 0; i < _countof(object3ds); i++)
        {
            UpdateObject3d(&object3ds[i], matView, matProjection);
        }

        // バックバッファの番号を取得（2つなので0番か1番）
        UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

        // １．リソースバリアで書き込み可能に変更
        D3D12_RESOURCE_BARRIER barrierDesc{};
        barrierDesc.Transition.pResource = backBuffers[bbIndex].Get(); // バックバッファを指定
        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;      // 表示状態から
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; // 描画状態へ
        commandList->ResourceBarrier(1, &barrierDesc);

        // ２．描画先の変更
        // レンダーターゲットビューのハンドルを取得
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += bbIndex * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
        // 深度ステンシルビュー用デスクリプタヒープのハンドルを取得
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        // ３．画面クリア           R     G     B    A
        FLOAT clearColor[] = { 0.1f,0.25f, 0.5f,0.0f }; // 青っぽい色
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // ４．描画コマンドここから
        // ビューポート設定コマンド
        D3D12_VIEWPORT viewport{};
        viewport.Width = window_width;
        viewport.Height = window_height;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        // ビューポート設定コマンドを、コマンドリストに積む
        commandList->RSSetViewports(1, &viewport);

        // シザー矩形
        D3D12_RECT scissorRect{};
        scissorRect.left = 0;                                       // 切り抜き座標左
        scissorRect.right = scissorRect.left + window_width;        // 切り抜き座標右
        scissorRect.top = 0;                                        // 切り抜き座標上
        scissorRect.bottom = scissorRect.top + window_height;       // 切り抜き座標下
        // シザー矩形設定コマンドを、コマンドリストに積む
        commandList->RSSetScissorRects(1, &scissorRect);
        // プリミティブ形状の設定コマンド
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 三角形リスト
        // パイプラインステートとルートシグネチャの設定コマンド
        commandList->SetPipelineState(pipelineState.Get());
        commandList->SetGraphicsRootSignature(rootSignature.Get());
        // 頂点バッファビューの設定コマンド
        commandList->IASetVertexBuffers(0, 1, &vbView);
        // インデックスバッファビューの設定コマンド
        commandList->IASetIndexBuffer(&ibView);
        // 定数バッファビュー(CBV)の設定コマンド
        commandList->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());
        // SRVヒープの設定コマンド
        ID3D12DescriptorHeap* descHeaps[] = {srvHeap.Get()};
        commandList->SetDescriptorHeaps(1, descHeaps);
        // SRVヒープの先頭ハンドルを取得（SRVを指しているはず）
        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
        srvGpuHandle.ptr += textureIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // SRVヒープの先頭にあるSRVをルートパラメータ1番に設定
        commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);

        // 全オブジェクトについて処理
        for (int i = 0; i < _countof(object3ds); i++)
        {
            DrawObject3d(&object3ds[i], commandList.Get(), vbView, ibView, _countof(indices));
        }

        // ４．描画コマンドここまで

        // ５．リソースバリアを戻す
        barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; // 描画状態から
        barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;        // 表示状態へ
        commandList->ResourceBarrier(1, &barrierDesc);

        // 命令のクローズ
        result = commandList->Close();
        assert(SUCCEEDED(result));
        // コマンドリストの実行
        ID3D12CommandList* commandLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, commandLists);

        // 画面に表示するバッファをフリップ（裏表の入替え）
        result = swapChain->Present(1, 0);
        assert(SUCCEEDED(result));

        // コマンドの実行完了を待つ
        commandQueue->Signal(fence.Get(), ++fenceVal);
        if (fence->GetCompletedValue() != fenceVal) {
            HANDLE event = CreateEvent(nullptr, false, false, nullptr);
            fence->SetEventOnCompletion(fenceVal, event);
            WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        // キューをクリア
        result = commandAllocator->Reset();
        assert(SUCCEEDED(result));
        // 再びコマンドリストを貯める準備
        result = commandList->Reset(commandAllocator.Get(), nullptr);
        assert(SUCCEEDED(result));

        // DirectX毎フレーム処理　ここまで

    }

    // ウィンドウクラスを登録解除
    UnregisterClass(w.lpszClassName, w.hInstance);

    return 0;
}
