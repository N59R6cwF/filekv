// dllmain.cpp : Defines the entry point for the DLL application.

#include "framework.h"

long SimpleHash(const char* str)
{
	long h = 0;

	while (*str != '\0')
	{
		h += (*str) * 7;
		h >>= (*str) % 3;

		str++;
	}

	return h;
}

void* NewFileKVWithDefaultHash(const _TCHAR* file)
{
    return NewFileKV(file, SimpleHash);
}

void FileKVFree(void* fkv) 
{
    FileKV_Free(fkv);
}

void FileKVPut(void* fkv, const char* key, const char* val)
{
    FileKV_Put(fkv, key, val);
}

void FileKVRemove(void* fkv, const char* key)
{
    FileKV_Remove(fkv, key);
}

void FileKVForeach(void* fkv, ForeachKVCallback callback)
{
    FileKV_Foreach(fkv, callback);
}

void FileKVFlush(void* fkv)
{
    FileKV_Flush(fkv);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

