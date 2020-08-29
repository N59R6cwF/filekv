#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include "..\\filekv.h"


#ifdef FILEKVDLL_EXPORTS
#define FILEKVDLL_API __declspec(dllexport)
#else
#define FILEKVDLL_API __declspec(dllimport)
#endif

FILEKVDLL_API void* NewFileKVWithDefaultHash(const _TCHAR* file);
FILEKVDLL_API void FileKVFree(void* fkv);
FILEKVDLL_API void FileKVPut(void* fkv, const char* key, const char* val);
FILEKVDLL_API void FileKVRemove(void* fkv, const char* key);
FILEKVDLL_API void FileKVForeach(void* fkv, ForeachKVCallback callback);
FILEKVDLL_API void FileKVFlush(void* fkv);