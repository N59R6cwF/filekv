#pragma once

#include <tchar.h>
#include "kvhashtable.h"
#include "kvfilemapping.h"

typedef long (*HashFunc)(const char *key);

/* return non-zero to break */
typedef int (*ForeachKVCallback)(const char *Key, const char *Val);

typedef struct {
	const _TCHAR* IndexPath;
	KVFileMapping* IndexMapping;
	KVHashTable* IndexTable;

	const _TCHAR* DataPath;
	KVFileMapping* DataMapping;

	HashFunc hash;
} FileKV;

FileKV* NewFileKV(const _TCHAR* file, HashFunc hash);
const char* FileKV_Find(FileKV* fkv, const char* key);

#define FileKV_Count(kv_ptr)	((kv_ptr)->IndexTable->DataCount)

int FileKV_Put(FileKV* fkv, const char* key, const char* val);
int FileKV_Remove(FileKV* fkv, const char* key);
void FileKV_Foreach(FileKV* fkv, ForeachKVCallback callback);
void FileKV_Flush(FileKV* fkv);
void FileKV_Free(FileKV* fkv);


