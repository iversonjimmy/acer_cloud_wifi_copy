// RegKey.cpp : Defines the entry point for the console application.
//
#include "RegKey.h"
#include <Windows.h>
#include <tchar.h>
#include <list>
using namespace std;


LONG	writeRegistryValues(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegValue>& RegValues)
{
	HKEY  hkey;
	DWORD dwDisposition;
	DWORD dwResult = 0;
	DWORD value = 0;
	LONG rv = RegCreateKeyEx(hMainKey,
		                     lpSubKey,
							 0,
							 NULL,
							 0,
							 KEY_WRITE | KEY_WOW64_32KEY,
							 NULL,
							 &hkey,
							 &dwDisposition);

	if(ERROR_SUCCESS == rv) {
		list<RegValue>::iterator it;
		for(it = RegValues.begin(); it != RegValues.end(); it++){
			rv = RegSetValueEx(hkey,
			               (*it).name,
						   0,
						   (*it).type,
						   (PBYTE)&((*it).data),
						   (*it).size);
			
			if(ERROR_SUCCESS != rv) {
				break;
			}
		}
	}

	RegCloseKey(hkey);
	return	rv;
}

LONG   getRegistryValues(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegValue>& RegValues)
{
	HKEY	hKey;
	TCHAR	buffer[256];

	LONG rv = RegOpenKeyEx(
		hMainKey,
		lpSubKey,
		0,
		KEY_QUERY_VALUE   | KEY_WOW64_32KEY,
		&hKey
	);

	if (ERROR_SUCCESS == rv) {
		int i = 0;
		DWORD	bufferSize = 256;
		DWORD   type;
		BYTE	out_buffer[256];
		DWORD	outBufferSize = 256;
		RegValue regData;
		while (1){

			rv = RegEnumValue(hKey, i, buffer, &bufferSize, 0, &type, out_buffer, &outBufferSize);
			if (ERROR_SUCCESS != rv) break;
			memcpy(regData.data, out_buffer, sizeof(BYTE) * 256);
			regData.size = outBufferSize;
			regData.type = type;
			memcpy(regData.name, buffer, sizeof(TCHAR) * 256);
			RegValues.push_back(regData);

			i++;
			bufferSize = 256;
			outBufferSize = 256;
		}

	}

	if(hKey)
		RegCloseKey(hKey);

	return (ERROR_NO_MORE_ITEMS == rv) ? ERROR_SUCCESS : rv;
}

LONG	createRegistryKeys(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegKey>& RegValues)
{
	HKEY  hkey;
	DWORD dwDisposition;
	DWORD dwResult = 0;
	DWORD value = 0;
	HKEY keyResult;
	LONG rv = RegCreateKeyEx(hMainKey,
		                     lpSubKey,
							 0,
							 NULL,
							 0,
							 KEY_CREATE_SUB_KEY | KEY_WOW64_32KEY,
							 NULL,
							 &hkey,
							 &dwDisposition);

	if(ERROR_SUCCESS == rv) {
		list<RegKey>::iterator it;
		for(it = RegValues.begin(); it != RegValues.end(); it++){

			rv = RegCreateKey(hkey,
			               (*it).name,
						   &keyResult
						   );
			
			if(ERROR_SUCCESS != rv) {	
				break;
			}
		}
	}

	RegCloseKey(hkey);
	return	rv;
}

LONG   getRegistryKeys(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegKey>& RegValues)
{
	HKEY	hKey;

	LONG rv = RegOpenKeyEx(
		hMainKey,
		lpSubKey,
		0,
		KEY_ENUMERATE_SUB_KEYS    | KEY_WOW64_32KEY,
		&hKey
	);

	if (ERROR_SUCCESS == rv) {
		int i = 0;
		TCHAR	out_buffer[256];
		DWORD	outBufferSize = 256;
		RegKey regData;
		while (1){

			rv = RegEnumKey(hKey, i, out_buffer, outBufferSize);
			if (ERROR_SUCCESS != rv) break;
			memcpy(regData.name, out_buffer, sizeof(TCHAR) * 256);
			RegValues.push_back(regData);
			i++;
		}

	}

	if(hKey)
		RegCloseKey(hKey);

	return (ERROR_NO_MORE_ITEMS == rv) ? ERROR_SUCCESS : rv;
}

#if 0
//unit_test
int _tmain(int argc, _TCHAR* argv[])
{
	LONG	rv;
	std::list<RegValue> RegValues;
	RegValue	regData;

	LONG rv = getRegistryValues(HKEY_CURRENT_USER, _T("Console\\Git Bash"), RegValues);

	list<RegValue>::iterator it;

	for(it = RegValues.begin(); it != RegValues.end(); it++){
		_tprintf(_T("%s "), (*it).name);
		if(_tcsstr((*it).name, _T("Size"))) {
			printf("(!)");
		}

		switch((*it).type){
			case	REG_SZ:
				_tprintf(_T(" %s\n"), (LPCSTR)(*it).data);
				break;
			case	REG_DWORD:
				_tprintf(_T(" 0x%08X\n"), *((DWORD*)(*it).data));
				break;
			default:
				printf("other type\n");
				break;
			}

	}

	RegValues.clear();

	memset(regData.data, 0, sizeof(regData.data));
	regData.data[0] = 10;
	regData.type = REG_DWORD;
	regData.size = 4;
	_stprintf((TCHAR*)regData.name, _T("Test"));
	RegValues.push_back(regData);

	_stprintf((TCHAR*)regData.data, _T("SUMMER!"));
	regData.type = REG_SZ;
	regData.size = 8 * sizeof(TCHAR);
	_stprintf((TCHAR*)regData.name, _T("Test2"));
	RegValues.push_back(regData);
	LONG rv = writeRegistryValues(HKEY_CURRENT_USER, _T("Console\\Git Bash"), RegValues);
	
	RegKey	regKey;
	std::list<RegKey> RegKeys;

	rv = getRegistryKeys(HKEY_CURRENT_USER, _T("Console\\"), RegKeys);

	list<RegKey>::iterator it2;

	for(it2 = RegKeys.begin(); it2 != RegKeys.end(); it2++){
		_tprintf(_T("%s\n"), (*it2).name);
		

	}

	_stprintf(regKey.name, _T("TEST"));
	RegKeys.push_back(regKey);
	rv = createRegistryKeys(HKEY_CURRENT_USER, _T("Console\\"), RegKeys);

	return 0;
}
#endif
