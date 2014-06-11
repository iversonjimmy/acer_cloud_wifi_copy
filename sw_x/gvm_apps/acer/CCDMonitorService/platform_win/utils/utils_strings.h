#ifndef __UTILS_STRINGS__
#define __UTILS_STRINGS__

#pragma once
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <tchar.h>

wchar_t *chr2wchr(const char *buffer)
{
    size_t len = strlen(buffer);
    size_t wlen = MultiByteToWideChar(CP_UTF8, 0, (const char*)buffer, -1, NULL, 0);
    wchar_t *wBuf = (wchar_t*)malloc(wlen*sizeof(wchar_t));//new wchar_t[wlen];
    memset((LPVOID)wBuf, 0, wlen*sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, (const char*)buffer, int(len), wBuf, int(wlen));
    return wBuf;
}

char *wchr2chr(const wchar_t *buffer)
{
    size_t wlen = wcslen(buffer);
    size_t len = WideCharToMultiByte(CP_UTF8, NULL, buffer, -1, NULL, 0, NULL, FALSE);
    char *Buf = (char*)malloc(len);
    memset((LPVOID)Buf, 0, len);
    WideCharToMultiByte(CP_UTF8, 0, buffer, int(wlen), Buf, int(len), NULL, NULL);
    return Buf;
}

#endif
