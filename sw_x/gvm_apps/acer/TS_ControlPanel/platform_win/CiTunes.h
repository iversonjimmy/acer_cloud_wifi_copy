#include <string>
#include "iTunesCOMInterface.h"
#include "CommonTool.h"

#include <map> 
#include <utility>

#pragma once
class CiTunes
{
public:
	CiTunes(void);
	~CiTunes(void);

	
	void GetAllList(TContentMap* inlitem);

protected: 
	private:
		CComPtr<IiTunes> m_iITunes;
		void Init();
		void ReleaseiTunes();
		void GetTrackInfo(CComPtr<IITFileOrCDTrack> _IITTrack );
		void GetPlaylist();
		void GetMediaItems();

		std::string GetCurrentSongTitle_iTunes();
		 //std::vector<CLOUD_MEIDA_METADATA_STRUCT*> m_litem ;
		std::vector<CLOUD_MEIDA_METADATA_STRUCT*> m_lMusicAlbum ;
		//CList<CString,CString&>m_lAlbumName;

		HRESULT CiTunes::SaveiTunesArt(CComPtr<IITFileOrCDTrack> inTrack,CString csSavePath);
 		
		TContentMap* m_litem;

};

