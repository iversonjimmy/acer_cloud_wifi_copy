#include <wmp.h>
#include "CommonTool.h"

//#include <map> 
//#include <utility>

#pragma once

class CWMP
{
public:
	CWMP(void);
	~CWMP(void);

	void GetAllList(TContentMap* inlitem);

protected:
	private:
		CComPtr<IWMPPlayer> m_spPlayer;

		void InitWMP();
		void ReleaseWMPLibrary();
		void GetMediaItems();
		void GetPlaylist();
		void GetWMPitemFromList(MEDIA_TYPE inMediatype );
		void GetWMPitemFromList(CComPtr<IWMPPlaylist> inlist, bool bIsPlaylist);
		TContentMap* m_litem;
};

