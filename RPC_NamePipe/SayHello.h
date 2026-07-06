

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 10:14:07 2038
 */
/* Compiler settings for SayHello.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __SayHello_h__
#define __SayHello_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __ISayHello_FWD_DEFINED__
#define __ISayHello_FWD_DEFINED__
typedef interface ISayHello ISayHello;

#endif 	/* __ISayHello_FWD_DEFINED__ */


#ifndef __SayHelloServer_FWD_DEFINED__
#define __SayHelloServer_FWD_DEFINED__

#ifdef __cplusplus
typedef class SayHelloServer SayHelloServer;
#else
typedef struct SayHelloServer SayHelloServer;
#endif /* __cplusplus */

#endif 	/* __SayHelloServer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ISayHello_INTERFACE_DEFINED__
#define __ISayHello_INTERFACE_DEFINED__

/* interface ISayHello */
/* [unique][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_ISayHello;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("11112222-3333-4444-5555-666677778888")
    ISayHello : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SayHello( 
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR *greeting) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ISayHelloVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISayHello * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISayHello * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISayHello * This);
        
        DECLSPEC_XFGVIRT(ISayHello, SayHello)
        HRESULT ( STDMETHODCALLTYPE *SayHello )( 
            ISayHello * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR *greeting);
        
        END_INTERFACE
    } ISayHelloVtbl;

    interface ISayHello
    {
        CONST_VTBL struct ISayHelloVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISayHello_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ISayHello_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ISayHello_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ISayHello_SayHello(This,name,greeting)	\
    ( (This)->lpVtbl -> SayHello(This,name,greeting) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ISayHello_INTERFACE_DEFINED__ */



#ifndef __SayHelloLib_LIBRARY_DEFINED__
#define __SayHelloLib_LIBRARY_DEFINED__

/* library SayHelloLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_SayHelloLib;

EXTERN_C const CLSID CLSID_SayHelloServer;

#ifdef __cplusplus

class DECLSPEC_UUID("aaaa1111-2222-3333-4444-555566667777")
SayHelloServer;
#endif
#endif /* __SayHelloLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


