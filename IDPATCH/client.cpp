// client.cpp
// Chay tren MAY A (client). Ket noi toi server tren MAY B qua dia chi IP.
//
// Bien dich (MinGW):
//   g++ shared.cpp client.cpp -o client.exe -lole32 -loleaut32 -luuid -static -municode
//
// Usage:
//   client.exe <IP_may_B>
//   client.exe <IP_may_B> <username> <password>   (neu can dang nhap, 2 may khac workgroup/domain)
//
// Vi du:
//   client.exe 192.168.1.20
//   client.exe 192.168.1.20 Administrator MatKhau123!

#include "shared.h"
#include <iostream>
#include <string>

int wmain(int argc, wchar_t* argv[])
{
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

    // --- Cau hinh dia chi may server ---
    COSERVERINFO serverInfo = {0};
    serverInfo.pwszName = const_cast<LPWSTR>(serverIP);

    // --- Credential (neu co) ---
    // Dung thang COAUTHIDENTITY (kieu ma pAuthIdentityData yeu cau),
    // tranh dung SEC_WINNT_AUTH_IDENTITY_W + ep kieu AUTH_IDENTITY_STRUCT
    // vi kieu nay khong co san trong MinGW headers.
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
        std::wcout << L"[*] Dung danh tinh hien tai (khong nhap username/password)." << std::endl;
    }

    std::wcout << L"[*] Ket noi toi server tai: " << serverIP << std::endl;

    MULTI_QI mqi = {0};
    mqi.pIID = &IID_IDispatch;

    hr = CoCreateInstanceEx(
        CLSID_Greeter, nullptr, CLSCTX_REMOTE_SERVER,
        &serverInfo, 1, &mqi);

    if (FAILED(hr) || FAILED(mqi.hr))
    {
        std::wcerr << L"[!] CoCreateInstanceEx that bai." << std::endl;
        std::wcerr << L"    hr=0x" << std::hex << hr << L"  mqi.hr=0x" << mqi.hr << std::endl;

        if (hr == 0x800706BA || mqi.hr == 0x800706BA)
            std::wcerr << L"    -> RPC server unavailable: server.exe co dang chay khong? "
                           L"Firewall co chan port 135 khong?" << std::endl;
        else if (hr == 0x80070005 || mqi.hr == (HRESULT)0x80070005)
            std::wcerr << L"    -> Access Denied: kiem tra DCOM permission (dcomcnfg) "
                           L"hoac credential username/password." << std::endl;
        else if (hr == 0x80040154 || mqi.hr == 0x80040154)
            std::wcerr << L"    -> Class not registered: server.exe da chay va dang ky "
                           L"(voi quyen Administrator) tren may server chua?" << std::endl;

        CoUninitialize();
        return 1;
    }

    IDispatch* pDisp = static_cast<IDispatch*>(mqi.pItf);
    std::wcout << L"[+] Ket noi thanh cong!\n" << std::endl;

    // Gan credential vao chinh proxy nay, de MOI LAN GOI HAM SAU DO
    // (Invoke) deu mang theo dung quyen dang nhap - khong chi luc
    // CoCreateInstanceEx ban dau. Thieu buoc nay se bi Access Denied
    // khi goi ham dù activation da thanh cong.
    if (username && password)
    {
        hr = CoSetProxyBlanket(
            pDisp,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            nullptr,
            RPC_C_AUTHN_LEVEL_CONNECT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            &authIdentity,
            EOAC_NONE
        );
        if (FAILED(hr))
        {
            std::wcerr << L"[!] CoSetProxyBlanket that bai: 0x" << std::hex << hr << std::endl;
        }
        else
        {
            std::wcout << L"[+] Da gan credential vao proxy." << std::endl;
        }
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

        VARIANTARG arg;
        VariantInit(&arg);
        arg.vt = VT_BSTR;
        arg.bstrVal = bstrName;

        DISPPARAMS params = {0};
        params.rgvarg = &arg;
        params.cArgs = 1;

        VARIANT result;
        VariantInit(&result);

        hr = pDisp->Invoke(
            DISPID_SAYHELLO, IID_NULL, LOCALE_USER_DEFAULT,
            DISPATCH_METHOD, &params, &result, nullptr, nullptr);

        if (SUCCEEDED(hr) && result.vt == VT_BSTR)
        {
            std::wcout << L"Phan hoi tu server: " << result.bstrVal << std::endl;
        }
        else
        {
            std::wcerr << L"Goi SayHello that bai. HRESULT=0x" << std::hex << hr << std::endl;
        }

        VariantClear(&result);
        SysFreeString(bstrName);
    }

    pDisp->Release();
    CoUninitialize();
    return 0;
}