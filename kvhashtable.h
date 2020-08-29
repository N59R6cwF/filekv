#pragma once

typedef struct {
	unsigned int Position_InBytes;
	unsigned int Capacity_InBytes;
	unsigned int KVLength_InBytes;
	int Next_InEntrySize;

	int IsInFreeList;
	int Hash;
	int Ext3;
	int Ext4;
} KVHaskTableEntry;

typedef struct {
	int Next_InEntrySize;
} KVHashTableSlot;

/* returning non-0 means satisfied */
typedef int (*EntryCheckCallback)(KVHaskTableEntry* This, void* Arg);

typedef void* (*MemoryExpandFunc)(int ByBytes, void* Arg);

typedef struct {
	KVHashTableSlot* FreeList;
	KVHashTableSlot* SlotStart;
	int SlotCount;
	int LoadFactor;

	KVHaskTableEntry* EntryStart;
	int EntryCount_IncludingFreeAndStray;
	int DataCount;

	MemoryExpandFunc Expander;

	void* ExpanderArg;

	EntryCheckCallback FreeListCheckFunc;
	EntryCheckCallback FindCheckFunc;
} KVHashTable;

/* return non-zero to break */
typedef int (*HashTableForeachCallback)(
	KVHashTable* ht,
	KVHashTableSlot* slot,
	KVHaskTableEntry* Prevoius,
	KVHaskTableEntry* This,
	void* Arg
	);

KVHashTable* NewKVHashTable(
	KVHashTable* Placeholder,
	void* MemoryStart,
	int LoadFactor,
	MemoryExpandFunc Expander,
	void* ExpanderArg,
	EntryCheckCallback FreeListCheckFunc,
	EntryCheckCallback FindCheckFunc
);

#define KV_ENTRY_NEWLY_CREATED_NEXT_VALUE	(-2)
#define KV_ENTRY_IS_NEWLY_CREATED(e_ptr)	((e_ptr)->Next_InEntrySize == KV_ENTRY_NEWLY_CREATED_NEXT_VALUE)

KVHashTable* KVHashTable_ReFix(
	KVHashTable* ht,
	void* MemoryStart,
	MemoryExpandFunc Expander,
	void* ExpanderArg,
	EntryCheckCallback FreeListCheckFunc,
	EntryCheckCallback FindCheckFunc
);

KVHaskTableEntry* KVHashTable_Foreach(
	KVHashTable* ht,
	HashTableForeachCallback Callback,
	void* CallbackArg
);

KVHaskTableEntry* KVHashTable_Insert(KVHashTable* ht, KVHaskTableEntry* ToBeInserted_StrayEntry, long Hash);
KVHaskTableEntry* KVHashTable_Find(KVHashTable* ht, long hash, void* FindArg);
KVHaskTableEntry* KVHashTable_RemoveNode(KVHashTable* ht, long Hash, KVHaskTableEntry* ToBeRemoved);
KVHaskTableEntry* KVHashTable_GetOrNewEntry(KVHashTable* ht, void* CheckFuncArg);
KVHaskTableEntry* KVHaskTableEntry_GetEntry(KVHashTable* ht, int Index_InEntrySize);
void KVHashTable_GiveBackEntry(KVHashTable* ht, KVHaskTableEntry* entry);
void KVHashTable_Free(KVHashTable* List);

#define HASHTABLE_INITIAL_SLOTS (128-1)
#define HASHTABLE_INITIAL_SIZE (sizeof(KVHashTableSlot) * (HASHTABLE_INITIAL_SLOTS + 1))
