#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>

typedef struct {
	HANDLE FileHandle;
	int FileIsNewlyCreated;

	HANDLE MappingHandle;
	char* MappingStart;
	unsigned int MappingSize;
} KVFileMapping;

KVFileMapping* NewKVFileMapping(const _TCHAR* FilePath, unsigned int MaxFileSize);
int KVFileMapping_Expand(KVFileMapping* IndexMapping, unsigned int ExpandBy);
void KVFileMapping_Flush(KVFileMapping* IndexMapping);
void KVFileMapping_Free(KVFileMapping* IndexMapping);
