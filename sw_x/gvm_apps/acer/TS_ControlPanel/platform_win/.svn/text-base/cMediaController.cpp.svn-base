#include "StdAfx.h"
#include "cMediaController.h"

#pragma unmanaged
CMediaController::CMediaController(void)
{
	InitDB();

	m_WinLib = NULL;
	m_iTunes = NULL;
	m_WMP = NULL;
	m_litem = NULL;
	//m_iTunes = new CiTunes();
	//m_WMP = new CWMP();
	m_WinLib = new CWinLib();
	m_litem = new TContentMap;

	//SelectSource(true,true,true);


}


CMediaController::~CMediaController(void)
{
}

void CMediaController::InitDB()
{
	
	TCHAR szPath[MAX_PATH];
 	SHGetSpecialFolderPath(NULL,  szPath, CSIDL_APPDATA, NULL);
	//Init DB save path
	TCHAR szFolderPath[MAX_PATH];
	wsprintf(szFolderPath, _T("%s\\acer"), szPath);
	_tmkdir(szFolderPath);
	wsprintf(szFolderPath, _T("%s\\acer\\clearfi"), szPath);
	_tmkdir(szFolderPath);
	wsprintf(szFolderPath, _T("%s\\acer\\clearfi\\MediaAgent"), szPath);
	_tmkdir(szFolderPath);
	wsprintf(szFolderPath, _T("%s\\acer\\clearfi\\MediaAgent\\clearfiMediaAgendDB.db"), szPath);
 
	wsprintf(m_szDBPath, szFolderPath);

	ToMultiBytes(m_szDBPath, m_szDBPathA);
	
	m_db = new CppSQLite3DB();
	InitSqlite3Lib();

	OpenDb(m_db,m_szDBPathA);
}

void CMediaController::SelectSource(bool bWMP,bool biTunes,bool bWin7Lib)
{
	m_WinLib->GetAllList(m_litem);
	//m_iTunes->GetAllList(m_litem);
	//m_WMP->GetAllList(m_litem);
}
