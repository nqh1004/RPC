# SayHello qua Named Pipe (dua tren code MIDL cua ban)

## Những gì đã đổi

Chỉ **`server_idl.cpp`** bị sửa, `client_idl.cpp` và `SayHello.idl` **giữ nguyên**:

1. Thêm `#include <rpc.h>` + hằng số:
   ```cpp
   static const wchar_t* RPC_PROTSEQ       = L"ncacn_np";
   static const wchar_t* RPC_PIPE_ENDPOINT = L"\\pipe\\SayHelloPipe";
   ```

2. Trong `main()`, gọi `RpcServerUseProtseqEpW(L"ncacn_np", ..., L"\pipe\SayHelloPipe", ...)`
   **ngay sau `CoInitializeEx`, trước `CoInitializeSecurity`**. Đây là bước
   báo cho RPC runtime của tiến trình này mở thêm một listener trên named pipe;
   OXID Resolver (`RPCSS`) sẽ quảng cáo endpoint này cho client khi client hỏi.

3. Trong `RegisterCLSID()`, thêm ghi `REG_MULTI_SZ` value `Endpoints` dưới
   `HKLM\Software\Classes\AppID\{AppID}`:
   ```
   ncacn_np,\pipe\SayHelloPipe
   ```
   Value này không bắt buộc để client kết nối được (OXID Resolver đã đủ),
   nhưng giúp một số công cụ quản trị DCOM (`dcomcnfg`) hiển thị đúng cấu hình.

## Vì sao client không cần sửa

DCOM có cơ chế **OXID Resolver**: khi client gọi `CoCreateInstanceEx` nhắm tới
một máy từ xa, nó luôn hỏi RPCSS trên máy đó qua endpoint mapper (port 135)
"đối tượng này đang lắng nghe ở đâu?". RPCSS trả lời danh sách protocol/endpoint
mà server đã đăng ký qua `RpcServerUseProtseqEp*` — nếu server chỉ đăng ký
`ncacn_np`, client sẽ tự dùng named pipe mà không cần code gì thêm.

## Build lại

```bat
:: Server (may B)
midl SayHello.idl
g++ server_idl.cpp SayHello_i.c -o server.exe -lole32 -loleaut32 -luuid -lrpcrt4 -static

:: Client (may A) - khong doi
g++ client_idl.cpp SayHello_i.c -o client.exe -lole32 -loleaut32 -luuid -static -municode
```

## Chạy lại (thứ tự không đổi)

```bat
:: May B (server), Admin, 1 lan duy nhat:
server.exe /register

:: May B, chay phuc vu:
server.exe

:: May A (client):
client.exe /register        REM 1 lan duy nhat, Admin
client.exe <IP_may_B>
```

## Kiểm tra pipe đang mở

Trên máy B, sau khi chạy `server.exe`, dùng Process Explorer (Sysinternals) →
xem Handles của `server.exe` → sẽ thấy handle tới `\Device\NamedPipe\SayHelloPipe`.
Hoặc dùng lệnh:
```bat
netstat -a | findstr 445
```
để xác nhận SMB (port 445, cổng named pipe remote đi qua) đang lắng nghe.

## Firewall vẫn cần mở

Dù transport thật là named pipe, bước OXID resolution ban đầu vẫn đi qua
RPC Endpoint Mapper (port 135), còn dữ liệu named pipe thật sự đi qua SMB
(port 445). Cả hai đều phải mở trên máy B:
```bat
netsh advfirewall firewall add rule name="DCOM-135" dir=in action=allow protocol=TCP localport=135
netsh advfirewall firewall add rule name="SMB-445"  dir=in action=allow protocol=TCP localport=445
```
