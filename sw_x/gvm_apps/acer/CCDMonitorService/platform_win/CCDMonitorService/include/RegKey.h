#ifndef __REGKEY__FUNCTIONS
#define __REGKEY__FUNCTIONS

#include <Windows.h>
#include <tchar.h>
#include <list>

typedef struct _reg_value {
    TCHAR name[256];
    DWORD type;
    DWORD size;
    BYTE  data[256];
} RegValue;

typedef struct _reg_key {
    TCHAR name[256];
} RegKey;


LONG writeRegistryValues(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegValue>& RegValues);
LONG getRegistryValues(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegValue>& RegValues);

LONG createRegistryKeys(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegKey>& RegValues);
LONG getRegistryKeys(HKEY hMainKey, LPCTSTR lpSubKey, std::list<RegKey>& RegValues);



#endif
