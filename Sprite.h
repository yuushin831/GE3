#pragma once
#include<DirectXMath.h>
#include "SpriteCommon.h"


//スプライト
class Sprite
{
private:
	struct ConstBuffDataMaterial {

		DirectX::XMFLOAT4 color;//色(RGBA)
	};

	struct Vertex {
		DirectX::XMFLOAT3 pos;//xyz座標
		DirectX::XMFLOAT2 uv; //uv座標
	};
public://メンバ関数
	//初期化
	void Initialize(SpriteCommon*_spriteCommon);

	void Draw();
private:
	static const int verticeCount = 4;
	//頂点データ
	Vertex vertices[verticeCount] = {
	{{-0.5f,-0.5f,0.0f},{0.0f,1.0f}},//左下
	{{-0.5f,+0.5f,0.0f},{0.0f,0.0f}},//左上
	{{+0.5f,-0.5f,0.0f},{1.0f,1.0f}},//右下
	{{+0.5f,+0.5f,0.0f},{1.0f,0.0f}},//右下
	};

	SpriteCommon* spriteCommon = nullptr;

	DirectX::XMFLOAT4 color = { 1,0,0,0.5f };

	Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff;

	D3D12_VERTEX_BUFFER_VIEW vbView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffMaterial;
	ConstBuffDataMaterial *constMapMaterial = nullptr;


};

