#pragma once
#include<windows.h>
#include<wrl.h>

#define DIRECTINPUT_VERSION     0x0800   // DirectInputのバージョン指定
#include <dinput.h>
class Input
{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	void Initialize(HINSTANCE hInstance,HWND hwnd);

	void Update();
	bool PushKey(BYTE KeyNumber);
	bool TriggerKey(BYTE KeyNumber);

private:
	// キーボードデバイスの生成
	ComPtr<IDirectInputDevice8> keyboard;

	ComPtr<IDirectInput8> directInput;

	BYTE key[256] = {};
	BYTE keyPre[256] = {};
};
