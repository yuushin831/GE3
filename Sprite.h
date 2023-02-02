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

	struct ConstBufferDataTransform
	{
		DirectX::XMMATRIX mat;
	};

	struct Vertex {
		DirectX::XMFLOAT3 pos;//xyz座標
		DirectX::XMFLOAT2 uv; //uv座標
	};
	//構造体
public:
	enum VertexNumber
	{
		LB,	//左下
		LT,	//左上
		RB,	//右下
		RT,	//右上
	};
public://メンバ関数
	//初期化
	void Initialize(SpriteCommon*_spriteCommon);

	void Update();

	void Draw();

	//Getter
	const DirectX::XMFLOAT4 GetColor() const { return color; }
	const DirectX::XMFLOAT2& GetPosition() const { return position; }
	const float& GetRotationZ() const { return rotationZ; }
	const DirectX::XMFLOAT2& GetSize() const { return size; };

	//Setter
	void SetColor(DirectX::XMFLOAT4 color) { this->color = color; }
	
	void SetPosition(const DirectX::XMFLOAT2& position) { this->position = position; }
	void SetRotationZ(const float& rotationZ) { this->rotationZ = rotationZ;}
	void SetSize(const DirectX::XMFLOAT2& size) { this->size = size; }
private:
	

	SpriteCommon* spriteCommon = nullptr;

	DirectX::XMFLOAT4 color = { 1,0,0,0.5f };

	float rotationZ = 0.f;
	DirectX::XMFLOAT2 position = { 0.0f,0.0f };
	DirectX::XMFLOAT2 size = { 100.f,100.f };
	

	Vertex vertices[4];

	Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff;

	D3D12_VERTEX_BUFFER_VIEW vbView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffMaterial;
	ConstBuffDataMaterial *constMapMaterial = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffTransform;
	ConstBufferDataTransform* constMapTransform=nullptr;

};

