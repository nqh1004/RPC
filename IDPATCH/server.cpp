// server.cpp
// Chay tren MAY B (server). PHAI chay voi quyen Administrator vi ghi vao
// HKEY_LOCAL_MACHINE (dang ky toan he thong, de may khac qua mang co the
// thay duoc, khac voi ban local truoc chi ghi HKEY_CURRENT_USER).
//
// Bien dich (MinGW):
//   g++ shared.cpp server.cpp -o server.exe -lole32 -loleaut32 -luuid -static
//
// Chay: click phai server.exe -> "Run as administrator"

#include "shared.h"
#include <iostream>
#include <string>

// ----------------- CGreeter: implement IDispatch -----------------
class CGreeter : public IDispatch
{
private:
    LONG m_refCount;

public:
    CGreeter() : m_refCount(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == IID_IDispatch)
        {
            *ppv = static_cast<IDispatch*>(this);
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

    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctinfo) override
    {
        *pctinfo = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo** ppTInfo) override
    {
        *ppTInfo = nullptr;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetIDsOfNames(
        REFIID, LPOLESTR* rgszNames, UINT cNames, LCID, DISPID* rgDispId) override
    {
        if (cNames != 1) return DISP_E_UNKNOWNNAME;
        if (_wcsicmp(rgszNames[0], L"SayHello") == 0)
        {
            rgDispId[0] = DISPID_SAYHELLO;
            return S_OK;
        }
        rgDispId[0] = DISPID_UNKNOWN;
        return DISP_E_UNKNOWNNAME;
    }

    HRESULT STDMETHODCALLTYPE Invoke(
        DISPID dispIdMember, REFIID, LCID, WORD wFlags,
        DISPPARAMS* pDispParams, VARIANT* pVarResult,
        EXCEPINFO*, UINT*) override
    {
        if (dispIdMember != DISPID_SAYHELLO)
            return DISP_E_MEMBERNOTFOUND;
        if (!(wFlags & DISPATCH_METHOD))
            return DISP_E_MEMBERNOTFOUND;
        if (pDispParams->cArgs != 1 || pDispParams->rgvarg[0].vt != VT_BSTR)
            return DISP_E_BADPARAMCOUNT;

        BSTR name = pDispParams->rgvarg[0].bstrVal;
        std::wstring wname(name, SysStringLen(name));
        std::wstring reply = L"Hello " + wname;

        std::wcout << L"[Server] Nhan ten tu client: " << wname << std::endl;
        std::wcout << L"[Server] Tra ve: " << reply << std::endl;

        if (pVarResult)
        {
            VariantInit(pVarResult);
            pVarResult->vt = VT_BSTR;
            pVarResult->bstrVal = SysAllocString(reply.c_str());
        }
        return S_OK;
    }
};

// ----------------- CGreeterFactory -----------------
class CGreeterFactory : public IClassFactory
{
private:
    LONG m_refCount;

public:
    CGreeterFactory() : m_refCount(1) {}

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
        CGreeter* pGreeter = new CGreeter();
        HRESULT hr = pGreeter->QueryInterface(riid, ppv);
        pGreeter->Release();
        return hr;
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL) override { return S_OK; }
};

// ----------------- Dang ky he thong (HKEY_LOCAL_MACHINE) -----------------
// Khac ban local (HKEY_CURRENT_USER): ghi vao HKLM de MAY KHAC qua mang
// co the thay duoc entry nay khi SCM cua may nay tra registry cuc bo.
// Can chay voi quyen Administrator.
bool RegisterServerSystemWide()
{
    wchar_t clsidStr[64] = {0};
    StringFromGUID2(CLSID_Greeter, clsidStr, 64);

    wchar_t appidStr[64] = {0};
    StringFromGUID2(APPID_Greeter, appidStr, 64);

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

    // CLSID -> ten hien thi
    ok &= writeKey(L"Software\\Classes\\CLSID\\" + std::wstring(clsidStr),
                    L"Greeter COM Server");

    // CLSID -> duong dan exe (LocalServer32)
    ok &= writeKey(L"Software\\Classes\\CLSID\\" + std::wstring(clsidStr) + L"\\LocalServer32",
                    exePath);

    // CLSID -> lien ket toi AppID (de dcomcnfg quan ly quyen)
    ok &= writeKey(L"Software\\Classes\\CLSID\\" + std::wstring(clsidStr) + L"\\AppID",
                    appidStr);

    // AppID -> ten hien thi trong dcomcnfg
    ok &= writeKey(L"Software\\Classes\\AppID\\" + std::wstring(appidStr),
                    L"Greeter COM Server");

    return ok;
}

// ----------------- main -----------------
int main()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::wcerr << L"[Server] CoInitializeEx that bai." << std::endl;
        return 1;
    }

    // Cho phep client tu xa goi vao. psa=NULL nghia la dung quyen
    // duoc cau hinh trong dcomcnfg (AppID) hoac quyen mac dinh cua may.
    hr = CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_CONNECT,
        RPC_C_IMP_LEVEL_IDENTIFY,
        nullptr, EOAC_NONE, nullptr);
    if (FAILED(hr))
        std::wcerr << L"[Server] CoInitializeSecurity canh bao: 0x"
                    << std::hex << hr << std::endl;

    if (!RegisterServerSystemWide())
    {
        std::wcerr << L"[Server] Ghi registry HKLM that bai.\n"
                       L"[Server] Ban co dang chay voi quyen Administrator khong?"
                    << std::endl;
        CoUninitialize();
        return 1;
    }
    std::wcout << L"[Server] Da dang ky he thong (HKLM) thanh cong." << std::endl;

    CGreeterFactory* pFactory = new CGreeterFactory();
    DWORD dwRegister = 0;
    hr = CoRegisterClassObject(
        CLSID_Greeter, pFactory, CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE, &dwRegister);

    if (FAILED(hr))
    {
        std::wcerr << L"[Server] CoRegisterClassObject that bai. HRESULT=0x"
                    << std::hex << hr << std::endl;
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