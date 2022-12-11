#pragma once
#include <Windows.h>

class WinApp
{
public://静的メンバ関数
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public://定数
	//ウィンドウ横幅
	static const int window_width = 1280;
	//ウィンドウ縦幅
	static const int window_height = 720;
public://メンバ関数
	//初期化
	void Initialize();
	//メッセージの処理
	bool ProcessMessage();
	//終了
	void Finalize();
	//getter
	HWND GetHwnd()const { return hwnd; }
	HINSTANCE GetHInstance()const { return w.hInstance; }
private:
	//ウィンドウハンドル
	HWND hwnd = nullptr;
	// ウィンドウクラスの設定
	WNDCLASSEX w{};
};
