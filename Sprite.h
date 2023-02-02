#pragma once
#include<DirectXMath.h>
#include "SpriteCommon.h"


//�X�v���C�g
class Sprite
{
private:
	

	struct ConstBuffDataMaterial {

		DirectX::XMFLOAT4 color;//�F(RGBA)
	};

	struct ConstBufferDataTransform
	{
		DirectX::XMMATRIX mat;
	};

	struct Vertex {
		DirectX::XMFLOAT3 pos;//xyz���W
		DirectX::XMFLOAT2 uv; //uv���W
	};
	//�\����
public:
	enum VertexNumber
	{
		LB,	//����
		LT,	//����
		RB,	//�E��
		RT,	//�E��
	};
public://�����o�֐�
	//������
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

