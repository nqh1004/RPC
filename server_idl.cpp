// server.cpp
// Chay tren MAY B. Dung ISayHello (sinh tu MIDL) thay vi IDispatch tu viet
// tay - goi ham truc tiep qua vtable, khong can Invoke()/VARIANT/DISPPARAMS
// nua. Van marshal qua mang duoc nho [oleautomation] trong file .idl.
//
// Chuan bi truoc khi bien dich:
//   1. midl SayHello.idl   -> sinh SayHello.h, SayHello_i.c, SayHello.tlb
//   2. Copy ca 3 file do vao cung thu muc voi server.cpp
//
// Bien dich (MinGW):
//   g++ server.cpp SayHello_i.c -o server.exe -lole32 -loleaut32 -luuid -lrpcrt4 -static
//
// LUU Y: phai copy SayHello.tlb ra cung thu muc voi server.exe (ban compile
// khong nhung .tlb vao trong .exe, chuong trinh doc no luc chay de tu dang
// ky Automation Marshaler).
//
// Chay: click phai server.exe -> "Run as administrator"

#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <rpc.h>
#include <iostream>
#include <string>
#include "SayHello.h"   // sinh tu MIDL - khai bao ISayHello, CLSID_SayHelloServer, IID_ISayHello, LIBID_SayHelloLib

// ---- MOI: cau hinh RPC transport = named pipe thay vi ALPC/TCP mac dinh ----
// Endpoint day du phai co tien to "\pipe\" khi truyen cho RpcServerUseProtseqEpW.
static const wchar_t* RPC_PROTSEQ       = L"ncacn_np";
static const wchar_t* RPC_PIPE_ENDPOINT = L"\\pipe\\SayHelloPipe";

// APPID rieng (tu dat, khong lien quan MIDL) - de quan ly quyen qua dcomcnfg
// {A1B2C3D5-1111-2222-3333-444455556667}
static const GUID APPID_SayHello =
{ 0xA1B2C3D5, 0x1111, 0x2222, { 0x33, 0x33, 0x44, 0x44, 0x55, 0x55, 0x66, 0x67 } };

// CLSID cua Automation Marshaler chuan cua Windows - co san trong moi may,
// khong phai tu dinh nghia. Dung de gan cho Interface\{IID}\ProxyStubClsid32
// -> bao Windows "dung universal marshaler cho interface nay, khong can
// proxy/stub tu bien".
static const wchar_t* AUTOMATION_MARSHALER_CLSID = L"{00020424-0000-0000-C000-000000000046}";

// ----------------- CSayHello: implement ISayHello -----------------
class CSayHello : public ISayHello
{
private:
    LONG m_refCount;

public:
    CSayHello() : m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == IID_ISayHello)
        {
            *ppv = static_cast<ISayHello*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }

    ULONG STDMETHODCALLTYPE Release() override
    {
        LONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) delete this;
        return count;
    }

    // Ham thuc su - goi truc tiep, khong can qua Invoke() nua!
    HRESULT STDMETHODCALLTYPE SayHello(BSTR name, BSTR* greeting) override
    {
        if (!greeting) return E_POINTER;

        std::wstring wname(name, SysStringLen(name));
        std::wstring reply = L"Hello " + wname;

        std::wcout << L"[Server] Nhan ten tu client: " << wname << std::endl;
        std::wcout << L"[Server] Tra ve: " << reply << std::endl;

        *greeting = SysAllocString(reply.c_str());
        return (*greeting != nullptr) ? S_OK : E_OUTOFMEMORY;
    }
};

// ----------------- CSayHelloFactory -----------------
class CSayHelloFactory : public IClassFactory
{
private:
    LONG m_refCount;

public:
    CSayHelloFactory() : m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        LONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) delete this;
        return count;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv) override
    {
        if (pOuter) return CLASS_E_NOAGGREGATION;
        CSayHello* pObj = new CSayHello();
        HRESULT hr = pObj->QueryInterface(riid, ppv);
        pObj->Release();
        return hr;
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL) override { return S_OK; }
};

// ----------------- Dang ky CLSID/AppID/LocalServer32 (giong ban truoc) -----------------
bool RegisterCLSID()
{
    wchar_t clsidStr[64] = {0};
    StringFromGUID2(CLSID_SayHelloServer, clsidStr, 64);

    wchar_t appidStr[64] = {0};
    StringFromGUID2(APPID_SayHello, appidStr, 64);

    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    HKEY hKey;
    bool ok = true;

    auto writeKey = [&](const std::wstring& path, const wchar_t* value) -> bool
    {
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, NULL, 0,
                             KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
            return false;
        LONG r = RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)value,
                                 (DWORD)((wcslen(value) + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return r == ERROR_SUCCESS;
    };

    // MOI: ghi REG_MULTI_SZ "Endpoints" duoi AppID, khai bao RPC dung named pipe.
    // Dinh dang moi chuoi con: "protseq,endpoint" (o day: "ncacn_np,\pipe\SayHelloPipe")
    auto writeMultiSz = [&](const std::wstring& path, const wchar_t* valueName,
                             const std::wstring& singleEntry) -> bool
    {
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, NULL, 0,
                             KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
            return false;

        std::wstring multiSz = singleEntry;
        multiSz.push_back(L'\0'); // ket thuc chuoi con
        multiSz.push_back(L'\0'); // ket thuc REG_MULTI_SZ

        LONG r = RegSetValueExW(hKey, valueName, 0, REG_MULTI_SZ,
                                 (const BYTE*)multiSz.data(),
                                 (DWORD)(multiSz.size() * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return r == ERROR_SUCCESS;
    };

    ok &= writeKey(L"Software\\Classes\\CLSID\\" + std::wstring(clsidStr),
                    L"SayHello COM Server");
    ok &= writeKey(L"Software\\Classes\\CLSID\\" + std::wstring(clsidStr) + L"\\LocalServer32",
                    exePath);
    ok &= writeKey(L"Software\\Classes\\CLSID\\" + std::wstring(clsidStr) + L"\\AppID",
                    appidStr);
    ok &= writeKey(L"Software\\Classes\\AppID\\" + std::wstring(appidStr),
                    L"SayHello COM Server");

    // MOI: khai bao endpoint named pipe cho AppID nay
    std::wstring endpointEntry = std::wstring(RPC_PROTSEQ) + L"," + RPC_PIPE_ENDPOINT;
    ok &= writeMultiSz(L"Software\\Classes\\AppID\\" + std::wstring(appidStr),
                        L"Endpoints", endpointEntry);

    return ok;
}

// ----------------- Dang ky Type Library (BUOC MOI so voi ban IDispatch) -----------------
// RegisterTypeLib() tu doc file .tlb, tu dong ghi Registry cho:
//   - HKCR\TypeLib\{LIBID}\...           -> vi tri file .tlb
//   - HKCR\Interface\{IID_ISayHello}\... -> gan ProxyStubClsid32 = Automation
//                                            Marshaler CLSID (o tren)
// Nho co buoc nay, Windows biet cach marshal ISayHello qua mang ma KHONG
// can build/dang ky mot proxy/stub DLL rieng.
bool RegisterTypeLibrary()
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
        std::wcerr << L"[Server] LoadTypeLibEx that bai (co file SayHello.tlb "
                       L"cung thu muc voi server.exe khong?). HRESULT=0x"
                    << std::hex << hr << std::endl;
        return false;
    }

    pTypeLib->Release();
    return true;
}

// ----------------- main -----------------
int main(int argc, char* argv[])
{
    // Che do 1: "server.exe /register"
    // Chi ghi Registry (CLSID/AppID/TypeLib) roi THOAT NGAY, khong chay
    // phuc vu gi ca. Chay CHI 1 LAN DUY NHAT, thu cong, voi quyen
    // Administrator that su (click phai -> Run as administrator).
    if (argc > 1 && std::string(argv[1]) == "/register")
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr)) { std::wcerr << L"CoInitializeEx that bai." << std::endl; return 1; }

        bool ok = RegisterCLSID();
        std::wcout << L"[Register] Dang ky CLSID/AppID: " << (ok ? L"OK" : L"THAT BAI") << std::endl;

        ok = ok && RegisterTypeLibrary();
        std::wcout << L"[Register] Dang ky Type Library: " << (ok ? L"OK" : L"THAT BAI") << std::endl;

        CoUninitialize();
        return ok ? 0 : 1;
    }

    // Che do 2: "server.exe" (khong tham so) - CHAY BINH THUONG
    // KHONG ghi Registry o day nua. Nho vay, du DCOM tu spawn tien trinh
    // nay duoi bat ky identity nao (khong du quyen admin thuc su), no
    // van chay duoc binh thuong vi khong can ghi HKLM nua - chi phuc vu
    // request dua tren Registry da duoc ghi san tu che do /register.
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::wcerr << L"[Server] CoInitializeEx that bai." << std::endl;
        return 1;
    }

    // MOI: mo RPC endpoint qua named pipe TRUOC khi CoInitializeSecurity.
    // Day la ky thuat de OXID Resolver (RPCSS) biet quang cao endpoint nay
    // cho client, ep DCOM dung named pipe thay vi ALPC/TCP mac dinh.
    RPC_STATUS rpcStatus = RpcServerUseProtseqEpW(
        (RPC_WSTR)RPC_PROTSEQ,
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        (RPC_WSTR)RPC_PIPE_ENDPOINT,
        nullptr);
    if (rpcStatus != RPC_S_OK && rpcStatus != RPC_S_DUPLICATE_ENDPOINT)
    {
        std::wcerr << L"[Server] RpcServerUseProtseqEpW that bai, ma loi: "
                    << rpcStatus << std::endl;
        CoUninitialize();
        return 1;
    }
    std::wcout << L"[Server] Da mo RPC endpoint tren named pipe: \\\\.\\pipe\\SayHelloPipe"
                << std::endl;

    hr = CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,  // MOI: khop voi client, dap ung DCOM hardening
        RPC_C_IMP_LEVEL_IDENTIFY,
        nullptr, EOAC_NONE, nullptr);
    if (FAILED(hr))
        std::wcerr << L"[Server] CoInitializeSecurity canh bao: 0x"
                    << std::hex << hr << std::endl;

    CSayHelloFactory* pFactory = new CSayHelloFactory();
    DWORD dwRegister = 0;
    hr = CoRegisterClassObject(
        CLSID_SayHelloServer, pFactory, CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE, &dwRegister);

    if (FAILED(hr))
    {
        std::wcerr << L"[Server] CoRegisterClassObject that bai. HRESULT=0x"
                    << std::hex << hr << std::endl;
        std::wcerr << L"[Server] Ban da chay 'server.exe /register' (1 lan, "
                       L"as Administrator) truoc do chua?" << std::endl;
        pFactory->Release();
        CoUninitialize();
        return 1;
    }

    std::wcout << L"[Server] San sang. Dang cho client tu MAY KHAC goi den..." << std::endl;
    std::wcout << L"[Server] (Ctrl+C hoac dong cua so de thoat)" << std::endl;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoRevokeClassObject(dwRegister);
    pFactory->Release();
    CoUninitialize();
    return 0;
}