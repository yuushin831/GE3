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

	const uint32_t GetTextureIndex() const { return textureIndex; }

	const DirectX::XMFLOAT2& GetPosition() const { return position; }
	const float& GetRotationZ() const { return rotationZ; }
	const DirectX::XMFLOAT2& GetSize() const { return size; };
	const DirectX::XMFLOAT2& GetAnchorPoint() const { return anchorPoint; };

	const DirectX::XMFLOAT2& GetTextureLeftTop() const { return textureLeftTop; };
	const DirectX::XMFLOAT2& GetTextureSize() const { return textureSize; };

	const bool& GetIsFlipX()  const { return IsFlipX; }
	const bool& GetIsFlipY()  const { return IsFlipY; }
	const bool& GetInvisble()  const { return IsInvisble; }

	//Setter
	void SetColor(DirectX::XMFLOAT4 color) { this->color = color; }

	void SetTextureIndex(uint32_t index) { this->textureIndex = index; }
	
	void SetPosition(const DirectX::XMFLOAT2& position) { this->position = position; }
	void SetRotationZ(const float& rotationZ) { this->rotationZ = rotationZ;}
	void SetSize(const DirectX::XMFLOAT2& size) { this->size = size; }
	void SetAnchorPoint(const DirectX::XMFLOAT2& anchorPoint) { this->anchorPoint = anchorPoint; }

	void SetTextureLeftTop(const DirectX::XMFLOAT2 LeftTop) { this->textureLeftTop = LeftTop; }
	void SetTextureSize(const DirectX::XMFLOAT2 Size) { this->textureSize = Size; }


	void SetIsFlipX(const bool& isFlipX) { this->IsFlipX = isFlipX; }
	void SetIsFlipY(const bool& isFlipY) { this->IsFlipY = isFlipY; }
	void SetInvisble(const bool& IsInvisble) { this->IsInvisble = IsInvisble; }



private:

	SpriteCommon* spriteCommon = nullptr;

	uint32_t textureIndex = 0;

	DirectX::XMFLOAT4 color = { 1,0,0,0.5f };

	float rotationZ = 0.f;
	DirectX::XMFLOAT2 position = { 0.0f,20.0f };
	DirectX::XMFLOAT2 size = { 100.f,100.f };
	DirectX::XMFLOAT2 anchorPoint = { 0.f,0.5f };

	DirectX::XMFLOAT2 textureLeftTop = { 250.0f,0.0f };
	DirectX::XMFLOAT2 textureSize = { 250.0f,100.0f };

	bool IsFlipX = false;
	bool IsFlipY = false;

	bool IsInvisble = false;
	

	Vertex vertices[4];

	Microsoft::WRL::ComPtr<ID3D12Resource> vertBuff;

	//Microsoft::WRL::ComPtr<ID3D12Resource> vertMap;
	Vertex* vertMap;

	D3D12_VERTEX_BUFFER_VIEW vbView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffMaterial;
	ConstBuffDataMaterial *constMapMaterial = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffTransform;
	ConstBufferDataTransform* constMapTransform=nullptr;


	
};

