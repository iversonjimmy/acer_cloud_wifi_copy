#include "CWMP.h"
#include "CiTunes.h"
#include "CWinLib.h"

#include "CommonTool.h"

#pragma once

class CMediaController
{
public:
	CMediaController(void);
	~CMediaController(void);

	CiTunes* m_iTunes;
	CWMP* m_WMP;
	CWinLib * m_WinLib ;

	void SelectSource(bool bWMP,bool biTunes,bool bWin7Lib);
	TContentMap* m_litem;
 
	TCHAR m_szDBPath[MAX_PATH];
	char m_szDBPathA[MAX_PATH];
	CppSQLite3DB* m_db;
	void InitDB();

};

