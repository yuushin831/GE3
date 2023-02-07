#include "Sprite.h"

using namespace DirectX;
using namespace Microsoft::WRL;

void Sprite::Initialize(SpriteCommon* _spriteCommon)
{
	HRESULT result{};
	assert(_spriteCommon);
	spriteCommon = _spriteCommon;

	float left = (0.0f - anchorPoint.x) * size.x;
	float right = (1.0f - anchorPoint.x) * size.x;
	float top = (0.0f - anchorPoint.y) * size.y;
	float bottom = (1.0f - anchorPoint.y) * size.y;

	//���E���]
	if (IsFlipX)
	{
		left = -left;
		right = -right;
	}
	//�㉺���]
	if (IsFlipY)
	{
		top = -top;
		bottom = -bottom;
	}


	vertices[LB] = { {left,bottom,0.0f}, {0.0f,1.0f} };//����
	vertices[LT] = { {left,top,0.0f},    {0.0f,0.0f} };//����
	vertices[RB] = { {right,bottom,0.0f},{1.0f,1.0f} };//�E��
	vertices[RT] = { {right,top,0.0f},   {1.0f,0.0f} };//�E��
	
	//���_�f�[�^
	UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));

	//���_�o�b�t�@�̐ݒ�
	D3D12_HEAP_PROPERTIES heapProp{};//�q�[�v�ݒ�
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;//GPU�ւ̓]���p
	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB;//���_�f�[�^�S�̂̃T�C�Y
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//���_�o�b�t�@�̐���
	//ID3D12Resource* vertBuff = nullptr;
	result = spriteCommon->GetDirectXCommon()->GetDevice()->CreateCommittedResource(
		&heapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));

	//�]��
	//Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));
	//�S���_�ɑ΂���
	for (int i = 0; i < _countof(vertices); i++) {
		vertMap[i] = vertices[i];//���W���R�s�[
	}
	//�q���������
	vertBuff->Unmap(0, nullptr);

	////���_�o�b�t�@�r���[�̍쐬
	//D3D12_VERTEX_BUFFER_VIEW vbView{};
	
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeVB;
	vbView.StrideInBytes = sizeof(vertices[0]);
	
	// �}�e���A��
	//�q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES cbHeapProp{};
	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC cbResourceDesc{};
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBuffDataMaterial) + 0xff) & ~0xff;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//�萔�o�b�t�@�̐ݒ�
	result = spriteCommon->GetDirectXCommon()->GetDevice()->CreateCommittedResource(
		&cbHeapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffMaterial)
	);
	assert(SUCCEEDED(result));

	//�萔�o�b�t�@�̃}�b�s���O
	result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial);//�}�b�s���O
	assert(SUCCEEDED(result));

	constMapMaterial->color = color;
	
	//�s��

	//�q�[�v�ݒ�
	//D3D12_HEAP_PROPERTIES cbHeapProp{};
	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	//���\�[�X�ݒ�
	//D3D12_RESOURCE_DESC cbResourceDesc{};
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//�萔�o�b�t�@�̐ݒ�
	result = spriteCommon->GetDirectXCommon()->GetDevice()->CreateCommittedResource(
		&cbHeapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffTransform)
	);
	assert(SUCCEEDED(result));

	//�萔�o�b�t�@�̃}�b�s���O
	result = constBuffTransform->Map(0, nullptr, (void**)&constMapTransform);//�}�b�s���O
	assert(SUCCEEDED(result));

	//���[���h
	XMMATRIX matWoeld;
	matWoeld = XMMatrixIdentity();

	rotationZ = 0.f;
	position={ 200.0f,50.0f };

	//��]
	XMMATRIX matRot;
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(XMConvertToRadians(rotationZ));
	matWoeld *= matRot;

	//���s�ړ�
	XMMATRIX matTrans;
	matTrans = XMMatrixTranslation(position.x, position.y, 0.0f);
	matWoeld *= matTrans;



	//�ˉe
	XMMATRIX matProjection = XMMatrixOrthographicOffCenterLH(
		0.f, WinApp::window_width,
		WinApp::window_height, 0.f,
		0.0f, 1.0f
	);
	constMapTransform->mat = matWoeld * matProjection;

	


	
}

void Sprite::Update()
{
	
	float left = (0.0f - anchorPoint.x) * size.x;
	float right = (1.0f - anchorPoint.x) * size.x;
	float top = (0.0f - anchorPoint.y) * size.y;
	float bottom = (1.0f - anchorPoint.y) * size.y;

	//���E���]
	if (IsFlipX)
	{
		left = -left;
		right = -right;
	}
	//�㉺���]
	if (IsFlipY)
	{
		top = -top;
		bottom = -bottom;
	}


	vertices[LB] = { {left,bottom,0.0f},{0.0f,1.0f} };//����
	vertices[LT] = { {left,top,0.0f},{0.0f,0.0f} };//����
	vertices[RB] = { {right,bottom,0.0f},{1.0f,1.0f} };//�E��
	vertices[RT] = { {right,top,0.0f},{1.0f,0.0f} };//�E��

	//�]��
	//Vertex* vertMap = nullptr;
	HRESULT result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));
	//�S���_�ɑ΂���
	for (int i = 0; i < _countof(vertices); i++) {
		vertMap[i] = vertices[i];//���W���R�s�[
	}
	//�q���������
	vertBuff->Unmap(0, nullptr);


	constMapMaterial->color = color;

	//���[���h
	XMMATRIX matWoeld;
	matWoeld = XMMatrixIdentity();

	//��]
	XMMATRIX matRot;
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(XMConvertToRadians(rotationZ));
	matWoeld *= matRot;
	//���s�ړ�
	XMMATRIX matTrans;
	matTrans = XMMatrixTranslation(position.x, position.y, 0.0f);
	matWoeld *= matTrans;

	//�ˉe
	XMMATRIX matProjection = XMMatrixOrthographicOffCenterLH(
		0.f, WinApp::window_width,
		WinApp::window_height, 0.f,
		0.0f, 1.0f
	);
	constMapTransform->mat = matWoeld * matProjection;

}

void Sprite::Draw()
{
	if (IsInvisble)
	{
		return;
	}

	spriteCommon->SetTextureCommands(textureIndex);

	spriteCommon->GetDirectXCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);

	spriteCommon->GetDirectXCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());
	spriteCommon->GetDirectXCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(2, constBuffTransform->GetGPUVirtualAddress());
	


	spriteCommon->GetDirectXCommon()->GetCommandList()->DrawInstanced(_countof(vertices), 1, 0, 0);


}
