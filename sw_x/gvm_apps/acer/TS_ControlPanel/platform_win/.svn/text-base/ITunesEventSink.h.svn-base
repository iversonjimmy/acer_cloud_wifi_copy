#pragma once
#include "itunescominterface.h"
#include "resource.h"
#include "testitunesdlg.h"

class CITunesEventSink :public _IiTunesEvents
{
private:
	 long       m_dwRefCount;
	 // Pointer to type information.
	 ITypeInfo* m_pITypeInfo ;

public:
	CITunesEventSink(void)
	{
		m_dwRefCount=0;
	    ITypeLib* pITypeLib = NULL ;
        HRESULT	 hr = ::LoadRegTypeLib(LIBID_iTunesLib, 
		                      1, 5, // Major/Minor version numbers
		                      0x00, 
		                      &pITypeLib) ;
// Get type information for the interface of the object.
		hr = pITypeLib->GetTypeInfoOfGuid(DIID__IiTunesEvents,

		                                  &m_pITypeInfo) ;

		
		pITypeLib->Release() ;
	}
	~CITunesEventSink(void)
	{
	}
	ULONG STDMETHODCALLTYPE AddRef()
    {
        InterlockedIncrement(&m_dwRefCount);
        return m_dwRefCount;
    }
    
	ULONG STDMETHODCALLTYPE Release()
    {
        InterlockedDecrement(&m_dwRefCount);
	
		if (m_dwRefCount == 0)
		{
			delete this;
			return 0;
		}
		
		return m_dwRefCount;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
    {
        if ((iid == IID_IDispatch)||(iid == DIID__IiTunesEvents))
        {
            m_dwRefCount++;
            *ppvObject = this;//(_IiTunesEvents *)this;
            return S_OK;
        }

        if (iid == IID_IUnknown)
        {
            m_dwRefCount++;            
            *ppvObject = this;//(IUnknown *)this;
            return S_OK;
        }

        return E_NOINTERFACE;
    }
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctinfo)
	{
		return E_NOTIMPL;
    }
	HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {
	    return E_NOTIMPL;
    }
	   
 
	HRESULT __stdcall GetIDsOfNames(  
	const IID& iid,
	OLECHAR** arrayNames,
	UINT countNames,
	LCID,          // Localization is not supported.
	DISPID* arrayDispIDs)
{
	if (iid != IID_NULL)
	{
		return DISP_E_UNKNOWNINTERFACE ;
	}

	HRESULT hr = m_pITypeInfo->GetIDsOfNames(arrayNames,
	                                         countNames,
	                                         arrayDispIDs) ;
	return hr ;
}

    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid,
		   LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		   EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		

		CTestiTunesDlg * pWnd=(CTestiTunesDlg *)AfxGetMainWnd();

		if(dispidMember==1)//database refresh
		    pWnd->PostMessage (WM_REFRESH_MESSAGE);

		
		return S_OK;
		
	
	  

	}
  

};
