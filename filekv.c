#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include "filekv.h"

#define FILEKV_VERSION 1

typedef struct {
	int Version;

	unsigned int IndexLength_IncludingHeader;
	unsigned int DataLength_IncludingFreeEntry;

	KVHashTable IndexTable;

	long long int _Tail;
} Header;

#define ROUND_DOWN(val, base)	((val) / (base) * (base))
#define ROUND(val, base)		ROUND_DOWN((val) + (base) / 2, base)
#define ROUND_UP(val, base)		ROUND_DOWN((val) + (base) - 1, base)


#define HEADER_SIZE ROUND_UP(sizeof(Header), 16)

#define GET_HEADER(kv_ptr)	((Header *)((kv_ptr)->IndexMapping->MappingStart))

#define INDEX_FILE_SUFFIX (_T(".index"))
#define INDEX_FILE_SUFFIX_LENGTH ((_tcsclen(INDEX_FILE_SUFFIX) + 1) * sizeof(_TCHAR))

typedef struct {
	FileKV* fkv;
	const char* Key;
}EqualityHelper;

/* private */ int /* bool */ IsEntryValid(FileKV* fkv, KVHaskTableEntry* e)
{
	Header* h = GET_HEADER(fkv);

	if (!(
		e->Position_InBytes >= 0 &&
		e->Position_InBytes <= (h->DataLength_IncludingFreeEntry) &&

		e->Capacity_InBytes >= 0 &&
		e->Capacity_InBytes + e->Position_InBytes <= (h->DataLength_IncludingFreeEntry) &&

		e->KVLength_InBytes > 0 &&
		e->KVLength_InBytes <= e->Capacity_InBytes
		)
		) {
		return 0 != 0;
	}

	const char* KeyStart = fkv->DataMapping->MappingStart + e->Position_InBytes;
	int KeyLen_WithoutTerminal = strnlen(KeyStart, e->KVLength_InBytes);
	if (KeyLen_WithoutTerminal == e->KVLength_InBytes)
	{
		return 0 != 0;
	}

	const char* ValStart = KeyStart + (KeyLen_WithoutTerminal + 1);
	int ValMaxLen = e->KVLength_InBytes - (KeyLen_WithoutTerminal + 1);
	int ValLen_WithoutTerminal = strnlen(ValStart, ValMaxLen);
	if (ValLen_WithoutTerminal == ValMaxLen)
	{
		return 0 != 0;
	}

	return 0 == 0;
}

/* private */ int /* bool */ Equal(KVHaskTableEntry* This, void* Arg)
{
	EqualityHelper* helper = (EqualityHelper*)Arg;

	if (helper->fkv->DataMapping == NULL || !IsEntryValid(helper->fkv, This))
	{
		return 0;
	}

	const char* KeyStart = helper->fkv->DataMapping->MappingStart + This->Position_InBytes;
	return strcmp(KeyStart, helper->Key) == 0;
}

/* private */ int FreeEntryChecker(KVHaskTableEntry* This, void* Arg)
{
	unsigned int need = (int)Arg;
	return This->Capacity_InBytes >= need;
}

/* private */ void* IndexExpander(int ByBytes, void* Arg)
{
	FileKV* fkv = (FileKV*)Arg;
	Header* h = GET_HEADER(fkv);

	if (ByBytes <= 0)
	{
		unsigned int Minus = (unsigned int)(-ByBytes);
		if (h->IndexLength_IncludingHeader >= Minus)
		{
			h->IndexLength_IncludingHeader -= Minus;
		}

		return NULL;
	}

	unsigned int Added = (unsigned int)ByBytes;

	if (fkv->IndexMapping->MappingSize - h->IndexLength_IncludingHeader < Added)
	{
		int BytesToExpand = ROUND_UP(
			Added - (fkv->IndexMapping->MappingSize - h->IndexLength_IncludingHeader),
			512
		);

		if (KVFileMapping_Expand(fkv->IndexMapping, BytesToExpand) != 0)
		{
			return NULL;
		}

		KVHashTable_ReFix(
			fkv->IndexTable,
			((char*)h) + HEADER_SIZE, /* Don't just be (h + 1) */
			IndexExpander, fkv,
			FreeEntryChecker,
			Equal
		);
	}

	char* Here = fkv->IndexMapping->MappingStart + (h->IndexLength_IncludingHeader);
	h->IndexLength_IncludingHeader += Added;
	return Here;
}

FileKV* NewFileKV(const _TCHAR* file, HashFunc hash)
{
	FileKV* New = (FileKV*)malloc(sizeof(FileKV));
	if (New == NULL)
	{
		return NULL;
	}

	New->IndexPath = NULL;
	New->IndexMapping = NULL;
	New->IndexTable = NULL;
	New->DataMapping = NULL;
	New->DataPath = NULL;

	New->hash = hash;

	_TCHAR* IndexFile = (_TCHAR*)malloc((_tcsclen(file) + 1) * sizeof(_TCHAR) + INDEX_FILE_SUFFIX_LENGTH + 8);
	if (IndexFile == NULL)
	{
		FileKV_Free(New);
		return NULL;
	}

	_tcscpy(IndexFile, file);
	_tcscat(IndexFile, INDEX_FILE_SUFFIX);

	New->IndexPath = IndexFile;

	unsigned int MinIndexLength = HASHTABLE_INITIAL_SIZE + HEADER_SIZE;

	New->IndexMapping = NewKVFileMapping(IndexFile, ROUND_UP(MinIndexLength, 512));
	if (New->IndexMapping == NULL)
	{
		FileKV_Free(New);
		return NULL;
	}

	Header* h = GET_HEADER(New);
	New->IndexTable = &(h->IndexTable);

	if (h->IndexLength_IncludingHeader > New->IndexMapping->MappingSize)
	{
		FileKV_Free(New);
		return NULL;
	}

	if (New->IndexMapping->FileIsNewlyCreated)
	{
		h->Version = FILEKV_VERSION;
		h->IndexLength_IncludingHeader = MinIndexLength;
		h->DataLength_IncludingFreeEntry = 0;
		h->_Tail = 0xABABABABABABABAB;

		if (NewKVHashTable(
			New->IndexTable,
			((char*)h) + HEADER_SIZE, /* Don't just be (h + 1) */
			1, /* Load Factor */
			IndexExpander, New,
			FreeEntryChecker,
			Equal
		) == NULL)
		{
			FileKV_Free(New);
			return NULL;
		}
	}
	else
	{
		if (KVHashTable_ReFix(
			New->IndexTable,
			((char*)h) + HEADER_SIZE, /* Don't just be (h + 1) */
			IndexExpander, New,
			FreeEntryChecker,
			Equal
		) == NULL)
		{
			FileKV_Free(New);
			return NULL;
		}

		New->DataMapping = NewKVFileMapping(file, 0);
		if (New->DataMapping == NULL)
		{
			FileKV_Free(New);
			return NULL;
		}

		if (h->DataLength_IncludingFreeEntry > New->DataMapping->MappingSize)
		{
			FileKV_Free(New);
			return NULL;
		}
	}

	New->DataPath = _tcsdup(file);
	if (New->DataPath == NULL)
	{
		FileKV_Free(New);
		return NULL;
	}


	return New;
}

/* private */ KVHaskTableEntry* FileKV_FindEntryWithHash(FileKV* fkv, const char* key, long hash)
{
	EqualityHelper helper;
	helper.Key = key;
	helper.fkv = fkv;

	return KVHashTable_Find(fkv->IndexTable, hash, &helper);
}

/* private */ KVHaskTableEntry* FileKV_FindEntry(FileKV* fkv, const char* key)
{
	return FileKV_FindEntryWithHash(fkv, key, (fkv->hash)(key));
}

const char* FileKV_Find(FileKV* fkv, const char* key)
{
	KVHaskTableEntry* e = FileKV_FindEntry(fkv, key);
	if (e == NULL)
	{
		return NULL;
	}

	const char* EntryStart = fkv->DataMapping->MappingStart + e->Position_InBytes;
	int KeyLength = strlen(EntryStart) + 1;
	const char* DataStart = EntryStart + KeyLength;

	return DataStart;
}

/* private */ char* ExpandDataZone(FileKV* fkv, int ByBytes)
{
	if (ByBytes <= 0)
	{
		return NULL;
	}

	unsigned int Added = (unsigned int)ByBytes;

	Header* h = GET_HEADER(fkv);

	if (fkv->DataMapping == NULL)
	{
		fkv->DataMapping = NewKVFileMapping(fkv->DataPath, ROUND_UP(Added, 512));
		if (fkv->DataMapping == NULL)
		{
			return NULL;
		}

		if (h->DataLength_IncludingFreeEntry > fkv->DataMapping->MappingSize)
		{
			KVFileMapping_Free(fkv->DataMapping);
			fkv->DataMapping = NULL;
			return NULL;
		}

		h->DataLength_IncludingFreeEntry = Added;

		return fkv->DataMapping->MappingStart;
	}
	else
	{
		if (fkv->DataMapping->MappingSize - h->DataLength_IncludingFreeEntry < Added)
		{
			int BytesToExpand = ROUND_UP(
				Added - (fkv->DataMapping->MappingSize - h->DataLength_IncludingFreeEntry),
				512
			);

			if (KVFileMapping_Expand(fkv->DataMapping, BytesToExpand) != 0)
			{
				return NULL;
			}
		}

		char* Here = fkv->DataMapping->MappingStart + (h->DataLength_IncludingFreeEntry);
		h->DataLength_IncludingFreeEntry += Added;
		return Here;
	}
}

int FileKV_Put(FileKV* fkv, const char* key, const char* val)
{
	long hash = (fkv->hash)(key);
	unsigned int ValSize = strlen(val) + 1;
	unsigned int KeySize = strlen(key) + 1;

	KVHaskTableEntry* past = FileKV_FindEntryWithHash(fkv, key, hash);
	if (past != NULL)
	{
		if (past->Capacity_InBytes >= KeySize + ValSize)
		{
			strcpy(fkv->DataMapping->MappingStart + past->Position_InBytes + KeySize, val);
			return 0;
		}
		else
		{
			if (KVHashTable_RemoveNode(fkv->IndexTable, hash, past) == NULL)
			{
				return -185;
			}
		}
	}

	KVHaskTableEntry* e = KVHashTable_GetOrNewEntry(fkv->IndexTable, (void*)(KeySize + ValSize));
	if (e == NULL)
	{
		return -175;
	}

	char* Here;

	if (KV_ENTRY_IS_NEWLY_CREATED(e))
	{
		int Capacity = ROUND_UP(KeySize + ValSize, 8);
		Here = ExpandDataZone(fkv, Capacity);
		if (Here == NULL)
		{
			// TODO: Deal with no disk space error
			KVHashTable_GiveBackEntry(fkv->IndexTable, e);
			return -188;
		}

		e->Capacity_InBytes = Capacity;
		e->Position_InBytes = Here - fkv->DataMapping->MappingStart;
	}
	else
	{
		Here = fkv->DataMapping->MappingStart + e->Position_InBytes;
	}

	e->KVLength_InBytes = KeySize + ValSize;

	strcpy(Here, key);
	strcpy(Here + KeySize, val);

	KVHashTable_Insert(fkv->IndexTable, e, hash);

	return 0;
}

int FileKV_Remove(FileKV* fkv, const char* key)
{
	KVHaskTableEntry* past = FileKV_FindEntry(fkv, key);
	if (past != NULL)
	{
		if (KVHashTable_RemoveNode(fkv->IndexTable, (fkv->hash)(key), past) == NULL)
		{
			return -234;
		}
	}

	return 0;
}

typedef struct {
	FileKV* fkv;
	ForeachKVCallback callback;
} ForeachKVHelper;

/* private */ int /* bool */ ForeachKVHelperCallback(KVHashTable* ht, KVHashTableSlot* _, KVHaskTableEntry* __, KVHaskTableEntry* This, void* Arg
)
{
	ForeachKVHelper* Helper = (ForeachKVHelper*)Arg;

	if (!IsEntryValid(Helper->fkv, This))
	{
		return 0;
	}

	const char* Key = Helper->fkv->DataMapping->MappingStart + This->Position_InBytes;
	const char* Val = Key + (strlen(Key) + 1);

	return (Helper->callback)(Key, Val);
}

void FileKV_Foreach(FileKV* fkv, ForeachKVCallback callback)
{
	if (fkv == NULL || callback == NULL)
	{
		return;
	}

	ForeachKVHelper Helper;
	Helper.callback = callback;
	Helper.fkv = fkv;

	KVHashTable_Foreach(fkv->IndexTable, ForeachKVHelperCallback, &Helper);
}

void FileKV_Free(FileKV* fkv)
{
	KVFileMapping_Free(fkv->DataMapping);
	KVFileMapping_Free(fkv->IndexMapping);
	free((void*)(fkv->IndexPath));
	free((void*)(fkv->DataPath));
	free(fkv);
}
