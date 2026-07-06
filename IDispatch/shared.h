// shared.h
// Dinh danh dung chung giua server (may B) va client (may A).
// Ca 2 file .exe deu #include header nay khi bien dich.
#pragma once
#include <windows.h>
#include <objbase.h>
#include <oleauto.h>

// CLSID: dinh danh COM class "Greeter" - client dung de tim server
extern const CLSID CLSID_Greeter;

// APPID: dinh danh "ung dung" trong DCOM Config (dcomcnfg.exe)
// -> cho phep cau hinh quyen Launch/Access bang giao dien do hoa de dang hon
extern const GUID APPID_Greeter;

// DISPID cua ham SayHello (thoa thuan rieng giua client/server nay)
#define DISPID_SAYHELLO 1
