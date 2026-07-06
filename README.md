# Nghiên cứu & Triển khai Windows COM/DCOM/RPC

Nghiên cứu chi tiết và triển khai các cơ chế Component Object Model (COM), Distributed COM (DCOM), và Remote Procedure Call (RPC) của Windows. Ba mô hình truyền thông phân biệt được khám phá trong bối cảnh offensive security: interface IDispatch truyền thống, vận chuyển named pipes, và endpoints dựa trên TCP.

## Tổng quan

### COM/DCOM/RPC là gì?

**COM (Component Object Model)**
- Chuẩn giao diện nhị phân Windows cho inter-process communication (IPC)
- Cơ chế RPC định hướng đối tượng với an toàn kiểu dữ liệu thông qua IDL (Interface Definition Language)
- Kiến trúc proxy/stub cho phép gọi phương thức độc lập với ngôn ngữ

**DCOM (Distributed COM)**
- Mở rộng của COM cho giao tiếp qua các máy từ xa
- Sử dụng RPC bên dưới để serialization và vận chuyển
- Kích hoạt được xử lý bởi Service Control Manager (SCM) trên máy đích
- Vector tấn công phổ biến: lateral movement, thực thi từ xa dưới ngữ cảnh SYSTEM

**RPC (Remote Procedure Call)**
- Stack protocol cấp thấp (MS-RPC, transports ncacn_*)
- Named pipes (`ncacn_np`), TCP (`ncacn_ip_tcp`), HTTP (`ncacn_http`)
- Hỗ trợ xác thực NTLM/Kerberos
- Vận chuyển cơ bản cho DCOM, WinRM, WMI, Task Scheduler APIs

## Cấu trúc Dự án

```
RPC/
├── IDispatch/           # Variant interface truyền thống
│   ├── client.cpp
│   ├── server.cpp
│   ├── shared.h
│   └── shared.cpp
│
├── RPC_NamePipe/        # DCOM qua SMB named pipes (firewall bypass)
│   ├── SayHello.idl     # Định nghĩa interface MIDL
│   ├── client_idl.cpp   # Custom interface (type-safe)
│   ├── server_idl.cpp
│   ├── SayHello.h       # Generated header
│   ├── SayHello.tlb     # Type library (runtime dependency)
│   └── ...
│
├── RPC_TCP/             # TCP-based endpoint (standard RPC)
│   ├── client.cpp
│   ├── server.cpp
│   └── ...
│
└── README.md
```

## Các Variant Triển khai

### 1. IDispatch (Truyền thống)
- **Vận chuyển:** DCOM proxy/stub DLL
- **Interface:** `IDispatch` (dynamic dispatch, dựa trên VARIANT)
- **Ưu điểm:** Đơn giản, được tài liệu hóa tốt, không cần biên dịch IDL
- **Nhược điểm:** Overhead VARIANT lúc runtime, kém type-safe, yêu cầu đăng ký DLL proxy/stub
- **Trường hợp sử dụng:** COM tương thích legacy

**Đặc điểm chính:**
```cpp
// Invoke() với DISPPARAMS/VARIANT
HRESULT Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, 
               WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, ...);
```

### 2. RPC NamePipe (Bypass Firewall)
- **Vận chuyển:** Named pipes qua SMB (port 445)
- **Interface:** Custom [oleautomation] IDL interface (vtable-based)
- **Xác thực:** NTLM/Kerberos qua SMB
- **Ưu điểm:** Tránh cổng RPC ephemeral (135, 1024+); firewall evasion; gọi phương thức type-safe
- **Nhược điểm:** Yêu cầu đăng ký Type Library (.tlb) trên cả hai máy; truy cập SMB port 445

**Đặc điểm chính:**
```cpp
// Gọi vtable trực tiếp (không overhead Invoke)
HRESULT SayHello(BSTR name, BSTR* greeting);

// Vận chuyển: ncacn_np (named pipes)
// Đường: \\target\pipe\ole\...  →  SMB 445
```

**Tác động Firewall:**
- Bỏ qua quy tắc chặn cổng RPC ephemeral
- Yêu cầu truy cập SMB (445) - thường mở cho file sharing
- Hữu ích trong mạng phân đoạn với lọc dựa trên port

### 3. RPC TCP (Standard Endpoint)
- **Vận chuyển:** TCP/IP (port 135 + ephemeral, hoặc fixed endpoint)
- **Interface:** Custom interface được sinh bởi MIDL
- **Ưu điểm:** Direct RPC, không phụ thuộc SMB, explicit endpoint binding
- **Nhược điểm:** Yêu cầu port 135 và high RPC ports (visibility firewall)
- **Trường hợp sử dụng:** DCOM cross-network, dễ phát hiện bằng forensics

**Đặc điểm chính:**
```cpp
// Đăng ký RPC endpoint chuẩn
RPC_STATUS status = RpcServerUseProtseqEp(
    (unsigned char*)"ncacn_ip_tcp",
    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
    (unsigned char*)"4567"  // Fixed port
);
```

## Khám phá Kỹ thuật

### MIDL (Microsoft Interface Definition Language)
Sinh proxy/stub code từ định nghĩa `.idl`:
```idl
[oleautomation, uuid(...)]
interface ISayHello : IUnknown {
    HRESULT SayHello([in] BSTR name, [out, retval] BSTR* greeting);
}
```

File được sinh ra:
- `SayHello.h` - Định nghĩa interface (vtable layout)
- `SayHello_i.c` - Định nghĩa IID/CLSID
- `SayHello_p.c` - Proxy/stub marshaling code
- `SayHello.tlb` - Type library (metadata interface nhị phân)

### Đăng ký Type Library
Quan trọng cho DCOM activation - cho phép SCM định vị marshaler:
```cpp
LoadTypeLibEx(tlbPath.c_str(), REGKIND_REGISTER, &pTypeLib);
// Registry: HKCR\Interface\{IID_ISayHello}\ProxyStubClsid32
//    = {00020424-0000-0000-C000-000000000046}  (Automation Marshaler)
```

### Xác thực & Token Filtering
Truy cập từ xa với local account yêu cầu:
```registry
HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System
  LocalAccountTokenFilterPolicy = 1  (Cho phép local accounts mà không cần admin elevation)
```

## Biên dịch & Triển khai

### Yêu cầu tiên quyết
- **Compiler:** MinGW/MSYS2 UCRT64
- **Tools:** MIDL compiler, RPC headers (`rpc.h`, `rpcndr.h`)
- **Libraries:** `-lole32 -loleaut32 -luuid`

### Biên dịch (MinGW)
```bash
# Sinh MIDL stubs (RPC_NamePipe only)
midl SayHello.idl

# Build Server
g++ server.cpp SayHello_i.c -o server.exe \
    -lole32 -loleaut32 -luuid -static -municode

# Build Client  
g++ client.cpp SayHello_i.c -o client.exe \
    -lole32 -loleaut32 -luuid -static -municode
```

### Đăng ký (RPC_NamePipe variant)
```cmd
# Máy Server (as Administrator)
server.exe /register          # Ghi CLSID/AppID/TypeLib vào registry

# Máy Client (as Administrator)  
client.exe /register          # Đăng ký Type Library cho marshaling
```

### Thực thi
```cmd
# Server - chờ DCOM activations
server.exe

# Client - remote instantiation + method call
client.exe 10.1.1.133 localadmin password123
```

## Forensics & Phát hiện

### Event Viewer - DistributedCOM
Giám sát các nỗ lực kích hoạt:
```
Source: DistributedCOM
Event ID: 10016 (Access Denied)
Event ID: 10000 (Successful activation)
Details: Target CLSID, Machine, User, Interface
```

### Chỉ báo Mạng

**NamePipe (SMB 445):**
```
SMB2 Tree Connect: \\target\IPC$
SMB2 Named Pipe: \pipe\ole\...
NTLM authentication handshake
```

**TCP (135 + ephemeral):**
```
RPC Endpoint Mapper: 135
Dynamic endpoints: 1024-65535
DCOM Service Control Manager (SCM) negotiation
```

### Registry Artifacts
```
HKLM\SYSTEM\CurrentControlSet\Services\RemoteRegistry  (activation logging)
HKLM\SOFTWARE\Classes\CLSID\{...}\LocalServer32        (server path)
HKLM\SOFTWARE\Classes\AppID\{...}                      (AppID config)
```

## Các Vấn đề Đã biết & Giải pháp

### Thử thách: `0x800706BA` (RPC_S_SERVER_UNAVAILABLE)
**Nguyên nhân gốc:** SCM không thể spawn server process hoặc DCOM activation timeout  
**Chẩn đoán:**
- Event Viewer DistributedCOM source tại thời điểm activation
- Xác minh đường dẫn `server.exe` trong registry CLSID\LocalServer32
- Kiểm tra `LocalAccountTokenFilterPolicy` cho truy cập local account
- Wireshark SMB2 capture lúc lỗi xảy ra

**Giải pháp:**
```batch
REM Đảm bảo server executable readable bởi activation account
REM Mở rộng DCOM activation timeout (dcomcnfg.exe → timeout settings)
REM Xác minh Type Library trên cả hai máy (client.exe /register)
```

### Thử thách: Triển khai Proxy/Stub
**Cách truyền thống:** Biên dịch `proxy/stub DLL`, đăng ký qua các máy  
**Cách hiện đại (NamePipe variant):** Sử dụng Automation Marshaler thông qua Type Library - không cần DLL

```registry
; Automation Marshaler CLSID (Microsoft-provided)
ProxyStubClsid32 = {00020424-0000-0000-C000-000000000046}
```

## Các Ứng dụng Offensive Security

### Lateral Movement
- **WMI:** Win32_Process.Create() thông qua WbemScripting.SWbemServices
- **Task Scheduler:** ITaskService.NewTask() (RPC_NamePipe variant trong repo này)
- **Services:** SCM RPC cho remote service enumeration/manipulation
- **Windows Management:** Registry, Event Log thông qua RPC

### Firewall Evasion
- NamePipe transport (SMB 445) tránh quy tắc dựa trên RPC port
- HTTP RPC endpoint (port 80/443) hòa lẫn với web traffic
- DCOM thông qua WinRM (5985/5986) trong một số tình huống

### Privilege Escalation
- Local DCOM → SYSTEM context execution
- Token filtering bypass: `LocalAccountTokenFilterPolicy=1`
- COM elevation moniker abuse

## Tài liệu tham khảo

- [MS-DCOM] Distributed Component Object Model (DCOM) Remote Protocol
- [MS-RPC] Remote Procedure Call (RPC) Protocol Specification
- [MITRE ATT&CK] TA0008 Lateral Movement, T1570 Lateral Tool Transfer
- Windows COM Security Architecture (MSDN)
- RPC/DCOM in Offensive Security (SpecterOps research)

## Tác giả & Bối cảnh

**Tiêu điểm Nghiên cứu:** Windows low-level internals, offensive security tooling  
**Môi trường Biên dịch:** MSYS2 UCRT64, MinGW gcc, Windows 11  
**Nền tảng Đích:** Windows Server 2019+, Windows 10/11  

---

*Cập nhật lần cuối: 2026*
