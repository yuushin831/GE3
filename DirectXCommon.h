#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>

#include"WinApp.h"
class DirectXCommon
{
public://�����o�֐�
	/// <summary>
	/// ������
	/// </summary>
	void Initialize(WinApp* winApp);

private:
	/// <summary>
	/// �f�o�C�X������
	/// </summary>
	void InitializeDevice();
	/// <summary>
	/// �R�}���h�֘A�̏�����
	/// </summary>
	void InitializeCommon();
	/// <summary>
	/// �X���b�v�`�F�[���̏�����
	/// </summary>
	void InitializeSwapchain();
	/// <summary>
	/// �����_�[�^�[�Q�b�g�r���[�̏�����
	/// </summary>
	void InitializeRenderTargetView();
	/// <summary>
	/// �[�x�o�b�t�@�̏�����
	/// </summary>
	void InitializeDepthBuffer();
	/// <summary>
	/// �t�F���X�̏�����
	/// </summary>
	void InitializeFence();

private:
	WinApp* winApp = nullptr;

	//�f�o�C�X
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	//�R�}���h�֘A
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	//�X���b�v�`�F�[��
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	//�����_�[�^�[�Q�b�g�r���[
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> backBuffers;
	//�[�x�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuff;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
	//�t�F���X
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
};
