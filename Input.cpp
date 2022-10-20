#include"Input.h"
#include<cassert>
//#include<wrl.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


//using namespace Microsoft::WRL;
void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
    HRESULT result;
    // DirectInputの初期化
    ComPtr<IDirectInput8> directInput;
    result = DirectInput8Create(
        hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
    assert(SUCCEEDED(result));



   
    result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    // 入力データ形式のセット
    result = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
    assert(SUCCEEDED(result));
    // 排他制御レベルのセット
    result = keyboard->SetCooperativeLevel(
        hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
    assert(SUCCEEDED(result));
}

void Input::Update()
{

    // キーボード情報の取得開始
    keyboard->Acquire();

    BYTE key[256] = {};

    // 全キーの入力状態を取得する
    keyboard->GetDeviceState(sizeof(key), key);
    
   


}
