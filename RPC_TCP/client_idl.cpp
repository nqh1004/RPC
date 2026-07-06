// client.cpp
// Chay tren MAY A. Goi ISayHello::SayHello() TRUC TIEP nhu 1 ham C++ binh
// thuong - khong con Invoke()/VARIANT/DISPPARAMS nhu ban IDispatch nua.
// Compiler kiem tra kieu du lieu luc bien dich (type-safe).
//
// Bien dich (MinGW):
//   g++ client.cpp SayHello_i.c -o client.exe -lole32 -loleaut32 -luuid -static -municode
//
// Usage:
//   client.exe <IP_may_B>
//   client.exe <IP_may_B> <username> <password>

#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <iostream>
#include <string>
#include "SayHello.h"

// Dang ky Type Library tren MAY NAY (may client) - can thiet de may nay
// biet cach "dung proxy" cho ISayHello khi nhan con tro interface tra ve
// tu may server. Interface tu dinh nghia (khac IDispatch co san) BAT BUOC
// phai duoc dang ky tren MOI may cham vao no, ca client lan server.
bool RegisterTypeLibraryLocal()
{
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring dir(exePath);
    size_t pos = dir.find_last_of(L"\\/");
    dir = (pos != std::wstring::npos) ? dir.substr(0, pos + 1) : L"";
    std::wstring tlbPath = dir + L"SayHello.tlb";

    ITypeLib* pTypeLib = nullptr;
    HRESULT hr = LoadTypeLibEx(tlbPath.c_str(), REGKIND_REGISTER, &pTypeLib);

    if (FAILED(hr))
    {
        std::wcerr << L"[Register] LoadTypeLibEx that bai (co file SayHello.tlb "
                       L"cung thu muc voi client.exe khong?). HRESULT=0x"
                    << std::hex << hr << std::endl;
        return false;
    }

    pTypeLib->Release();
    return true;
}

int wmain(int argc, wchar_t* argv[])
{
    // Che do dang ky: "client.exe /register"
    // Chi dang ky Type Library (ISayHello) tren may nay roi thoat, khong
    // ket noi gi ca. Chay 1 LAN DUY NHAT, as Administrator, TREN MAY A
    // (khac voi "server.exe /register" chay tren may B).
    if (argc > 1 && std::wstring(argv[1]) == L"/register")
    {
        bool ok = RegisterTypeLibraryLocal();
        std::wcout << L"[Register] Dang ky Type Library tren may nay: "
                    << (ok ? L"OK" : L"THAT BAI") << std::endl;
        return ok ? 0 : 1;
    }

    if (argc < 2)
    {
        std::wcout << L"Usage:\n"
                       L"  client.exe <IP_may_server>\n"
                       L"  client.exe <IP_may_server> <username> <password>\n";
        return 1;
    }

    const wchar_t* serverIP = argv[1];
    const wchar_t* username = (argc > 2) ? argv[2] : nullptr;
    const wchar_t* password = (argc > 3) ? argv[3] : nullptr;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::wcerr << L"CoInitializeEx that bai." << std::endl;
        return 1;
    }

    hr = CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_CONNECT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr);
    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
        std::wcerr << L"CoInitializeSecurity canh bao: 0x" << std::hex << hr << std::endl;

    COSERVERINFO serverInfo = {0};
    serverInfo.pwszName = const_cast<LPWSTR>(serverIP);

    COAUTHIDENTITY authIdentity = {0};
    COAUTHINFO authInfo = {0};

    if (username && password)
    {
        authIdentity.User = (USHORT*)username;
        authIdentity.UserLength = (ULONG)wcslen(username);
        authIdentity.Password = (USHORT*)password;
        authIdentity.PasswordLength = (ULONG)wcslen(password);
        authIdentity.Domain = (USHORT*)L"";
        authIdentity.DomainLength = 0;
        authIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        authInfo.dwAuthnSvc = RPC_C_AUTHN_WINNT;
        authInfo.dwAuthzSvc = RPC_C_AUTHZ_NONE;
        authInfo.pwszServerPrincName = nullptr;
        authInfo.dwAuthnLevel = RPC_C_AUTHN_LEVEL_CONNECT;
        authInfo.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
        authInfo.pAuthIdentityData = &authIdentity;
        authInfo.dwCapabilities = EOAC_NONE;

        serverInfo.pAuthInfo = &authInfo;
        std::wcout << L"[*] Dung tai khoan: " << username << std::endl;
    }
    else
    {
        std::wcout << L"[*] Dung danh tinh hien tai." << std::endl;
    }

    std::wcout << L"[*] Ket noi toi server tai: " << serverIP << std::endl;

    // Xin thang IID_ISayHello - khong con IID_IDispatch nua
    MULTI_QI mqi = {0};
    mqi.pIID = &IID_ISayHello;

    hr = CoCreateInstanceEx(
        CLSID_SayHelloServer, nullptr, CLSCTX_REMOTE_SERVER,
        &serverInfo, 1, &mqi);

    if (FAILED(hr) || FAILED(mqi.hr))
    {
        std::wcerr << L"[!] CoCreateInstanceEx that bai." << std::endl;
        std::wcerr << L"    hr=0x" << std::hex << hr << L"  mqi.hr=0x" << mqi.hr << std::endl;

        if (hr == 0x800706BA || mqi.hr == 0x800706BA)
            std::wcerr << L"    -> RPC server unavailable." << std::endl;
        else if (hr == 0x80070005 || mqi.hr == (HRESULT)0x80070005)
            std::wcerr << L"    -> Access Denied." << std::endl;
        else if (hr == 0x80040154 || mqi.hr == 0x80040154)
            std::wcerr << L"    -> Class not registered." << std::endl;

        CoUninitialize();
        return 1;
    }

    ISayHello* pSayHello = static_cast<ISayHello*>(mqi.pItf);
    std::wcout << L"[+] Ket noi thanh cong!\n" << std::endl;

    if (username && password)
    {
        hr = CoSetProxyBlanket(
            pSayHello, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
            RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE,
            &authIdentity, EOAC_NONE);
        if (SUCCEEDED(hr))
            std::wcout << L"[+] Da gan credential vao proxy." << std::endl;
    }

    std::wstring input;
    std::wcout << L"Nhap ten (go 'exit' de thoat):" << std::endl;
    while (true)
    {
        std::wcout << L"> ";
        std::getline(std::wcin, input);
        if (input == L"exit") break;
        if (input.empty()) continue;

        BSTR bstrName = SysAllocString(input.c_str());
        BSTR bstrGreeting = nullptr;

        // Goi TRUC TIEP - khong con Invoke()/VARIANT/DISPPARAMS nua!
        hr = pSayHello->SayHello(bstrName, &bstrGreeting);

        if (SUCCEEDED(hr) && bstrGreeting)
        {
            std::wcout << L"Phan hoi tu server: " << bstrGreeting << std::endl;
            SysFreeString(bstrGreeting);
        }
        else
        {
            std::wcerr << L"Goi SayHello that bai. HRESULT=0x" << std::hex << hr << std::endl;
        }

        SysFreeString(bstrName);
    }

    pSayHello->Release();
    CoUninitialize();
    return 0;
}