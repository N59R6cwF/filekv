#include <stdlib.h>
#include <string.h>
#include "kvhashtable.h"

#define _____ABS(x) ((x)<0 ? (-(x)) : (x))
#define CalcSlotOfHash(t_ptr, hash)	((t_ptr)->SlotStart + _____ABS(hash) % (t_ptr)->SlotCount)

#define EntryIndex(ht_ptr, e_ptr)		((e_ptr) - ((ht_ptr)->EntryStart))

#define ValidIndex_InEntrySize(t_ptr, i)	((i) >= 0 && (i) < (t_ptr)->EntryCount_IncludingFreeAndStray)

#define END_ENTRY_NEXT_VALUE	(-1)

#define SlotIsEmpty(e_ptr)		((e_ptr)->Next_InEntrySize == END_ENTRY_NEXT_VALUE)
#define EntryIsEnd(e_ptr)		((e_ptr)->Next_InEntrySize == END_ENTRY_NEXT_VALUE)
#define EntryHasNext(t_ptr, e_ptr)		ValidIndex_InEntrySize(t_ptr, (e_ptr)->Next_InEntrySize)

KVHashTable* KVHashTable_ReFix(
	KVHashTable* ht,
	void* MemoryStart,
	MemoryExpandFunc Expander,
	void* ExpanderArg,
	EntryCheckCallback FreeListCheckFunc,
	EntryCheckCallback FindCheckFunc
)
{
	if (MemoryStart == NULL ||
		Expander == NULL ||
		FreeListCheckFunc == NULL ||
		FindCheckFunc == NULL
		)
	{
		return NULL;
	}

	ht->FreeList = MemoryStart;
	ht->SlotStart = ht->FreeList + 1;
	ht->EntryStart = (KVHaskTableEntry*)(ht->SlotStart + ht->SlotCount);

	ht->Expander = Expander;
	ht->ExpanderArg = ExpanderArg;
	ht->FreeListCheckFunc = FreeListCheckFunc;
	ht->FindCheckFunc = FindCheckFunc;

	return ht;
}

/* private */ void HashTable_Expand(KVHashTable* ht);

/* private */ void ResetSlots(KVHashTable* ht)
{
	if (ht == NULL)
	{
		return;
	}

	KVHashTableSlot* s;
	for (s = ht->SlotStart; s != (KVHashTableSlot*)(ht->EntryStart); ++s)
	{
		s->Next_InEntrySize = END_ENTRY_NEXT_VALUE;
	}
}

KVHashTable* NewKVHashTable(
	KVHashTable* Placeholder,
	void* MemoryStart,
	int LoadFactor,
	MemoryExpandFunc Expander,
	void* ExpanderArg,
	EntryCheckCallback FreeListCheckFunc,
	EntryCheckCallback FindCheckFunc
)
{
	if (Placeholder == NULL ||
		MemoryStart == NULL ||
		LoadFactor < 0 ||
		Expander == NULL ||
		FreeListCheckFunc == NULL
		)
	{
		return NULL;
	}

	Placeholder->SlotCount = HASHTABLE_INITIAL_SLOTS;
	Placeholder->LoadFactor = LoadFactor;

	Placeholder->EntryCount_IncludingFreeAndStray = 0;
	Placeholder->DataCount = 0;

	if (KVHashTable_ReFix(Placeholder, MemoryStart, Expander, ExpanderArg, FreeListCheckFunc, FindCheckFunc) == NULL)
	{
		KVHashTable_Free(Placeholder);
		return NULL;
	}

	Placeholder->FreeList->Next_InEntrySize = END_ENTRY_NEXT_VALUE;
	ResetSlots(Placeholder);

	return Placeholder;
}

KVHaskTableEntry* KVHaskTableEntry_GetEntry(KVHashTable* ht, int Index_InEntrySize)
{
	if (ht == NULL || !ValidIndex_InEntrySize(ht, Index_InEntrySize))
	{
		return NULL;
	}

	KVHaskTableEntry* Next = ht->EntryStart + Index_InEntrySize;

	return (KVHaskTableEntry*)Next;
}

/* private */ KVHaskTableEntry* Next(KVHashTable* List, KVHaskTableEntry* This)
{
	if (This == NULL)
	{
		return NULL;
	}

	return KVHaskTableEntry_GetEntry(List, This->Next_InEntrySize);
}

/* private */ KVHaskTableEntry* Insert(KVHashTable* ht, KVHashTableSlot* slot, KVHaskTableEntry* ToBeInserted_StrayEntry, int NoExpand)
{
	if (ht == NULL ||
		slot == NULL ||
		ToBeInserted_StrayEntry == NULL
		)
	{
		return NULL;
	}

	ToBeInserted_StrayEntry->Next_InEntrySize = slot->Next_InEntrySize;
	slot->Next_InEntrySize = EntryIndex(ht, ToBeInserted_StrayEntry);

	ToBeInserted_StrayEntry->IsInFreeList = slot == ht->FreeList;
	if (ToBeInserted_StrayEntry->IsInFreeList)
	{

	}
	else
	{
		++(ht->DataCount);
	}

	if (!NoExpand)
	{
		HashTable_Expand(ht);
	}

	return ToBeInserted_StrayEntry;
}

/* private */ KVHaskTableEntry* KVHashTable_InsertInner(KVHashTable* ht, KVHaskTableEntry* ToBeInserted_StrayEntry, long Hash, int NoExpand)
{
	ToBeInserted_StrayEntry->Hash = Hash;
	return Insert(ht, CalcSlotOfHash(ht, Hash), ToBeInserted_StrayEntry, NoExpand);
}

KVHaskTableEntry* KVHashTable_Insert(KVHashTable* ht, KVHaskTableEntry* ToBeInserted_StrayEntry, long Hash)
{
	return KVHashTable_InsertInner(ht, ToBeInserted_StrayEntry, Hash, 0 != 0);
}

/* private */ KVHaskTableEntry* ForeachInSlot(
	KVHashTable* ht, KVHashTableSlot* slot,
	HashTableForeachCallback Callback,
	void* CallbackArg
)
{
	if (ht == NULL ||
		slot == NULL ||
		Callback == NULL)
	{
		return NULL;
	}

	if (SlotIsEmpty(slot))
	{
		return NULL;
	}

	KVHaskTableEntry* Previous = NULL;
	KVHaskTableEntry* This = KVHaskTableEntry_GetEntry(ht, slot->Next_InEntrySize);
	do
	{
		int r = Callback(ht, slot, Previous, This, CallbackArg);
		if (r != 0)
		{
			return This;
		}

		if (EntryIsEnd(This))
		{
			return NULL;
		}

		Previous = This;
		This = Next(ht, This);

#ifndef _DEBUG
		if (This == NULL)
		{
			return NULL;
		}
#endif
	} while (1);
}

KVHaskTableEntry* KVHashTable_Foreach(
	KVHashTable* ht,
	HashTableForeachCallback Callback,
	void* CallbackArg
)
{
	if (ht == NULL || Callback == NULL)
	{
		return NULL;
	}

	KVHaskTableEntry* This;
	for (This = ht->EntryStart; (This - ht->EntryStart) < ht->EntryCount_IncludingFreeAndStray; ++This)
	{
		if (This->IsInFreeList)
		{
			continue;
		}

		int r = Callback(ht, NULL, NULL, This, CallbackArg);
		if (r != 0)
		{
			return This;
		}
	}

	return NULL;
}

/* private */ int FindCallback(KVHashTable* ht, KVHashTableSlot* slot, KVHaskTableEntry* Prevoius, KVHaskTableEntry* This, void* Arg)
{
	return (ht->FindCheckFunc)(This, Arg);
}

KVHaskTableEntry* KVHashTable_Find(KVHashTable* ht, long hash, void* FindArg)
{
	KVHashTableSlot* s = CalcSlotOfHash(ht, hash);

	return ForeachInSlot(ht, s, FindCallback, FindArg);
}

/* private */ int RemoveNodeCallback(KVHashTable* ht, KVHashTableSlot* slot, KVHaskTableEntry* Prevoius, KVHaskTableEntry* This, void* Arg)
{
	KVHaskTableEntry* ToBeRemoved = (KVHaskTableEntry*)Arg;
	if (This == ToBeRemoved)
	{
		if (Prevoius == NULL)
		{
			slot->Next_InEntrySize = This->Next_InEntrySize;
		}
		else
		{
			Prevoius->Next_InEntrySize = This->Next_InEntrySize;
		}

		if (slot != ht->FreeList)
		{
			--(ht->DataCount);
			Insert(ht, ht->FreeList, ToBeRemoved, 0 == 0);
		}

		return 1;
	}

	return 0;
}

KVHaskTableEntry* KVHashTable_RemoveNode(KVHashTable* ht, long Hash, KVHaskTableEntry* ToBeRemoved)
{
	KVHashTableSlot* slot = CalcSlotOfHash(ht, Hash);

	if (ForeachInSlot(ht, slot, RemoveNodeCallback, ToBeRemoved) != NULL)
	{
		return ToBeRemoved;
	}
	else
	{
		return NULL;
	}
}

/* private */ int GetSuitableEntryInFreeListHelperCallback(KVHashTable* ht, KVHashTableSlot* slot, KVHaskTableEntry* Prevoius, KVHaskTableEntry* This, void* CallbackArg)
{
	if ((ht->FreeListCheckFunc)(This, CallbackArg))
	{
		RemoveNodeCallback(ht, slot, Prevoius, This, This);
		return 1;
	}

	return 0;
}

/* private */ KVHaskTableEntry* GetSuitableEntryInFreeList(KVHashTable* ht, void* CheckFuncArg)
{
	if (ht == NULL)
	{
		return NULL;
	}

	return ForeachInSlot(ht, ht->FreeList, GetSuitableEntryInFreeListHelperCallback, CheckFuncArg);
}

KVHaskTableEntry* KVHashTable_GetOrNewEntry(KVHashTable* ht, void* CheckFuncArg)
{
	if (ht == NULL)
	{
		return NULL;
	}

	KVHaskTableEntry* e = NULL;

	e = GetSuitableEntryInFreeList(ht, CheckFuncArg);
	if (e != NULL)
	{
		return e;
	}

	KVHaskTableEntry* ne = (ht->Expander)(sizeof(KVHaskTableEntry), ht->ExpanderArg);
	if (ne == NULL)
	{
		return NULL;
	}

	ne->Next_InEntrySize = KV_ENTRY_NEWLY_CREATED_NEXT_VALUE;
	ne->IsInFreeList = 0 == 0;

	++(ht->EntryCount_IncludingFreeAndStray);

	return ne;
}

#define HashTable_NeedExpand(t_ptr)	((t_ptr)->DataCount / (t_ptr)->SlotCount >= (t_ptr)->LoadFactor)

/* private */ void HashTable_Expand(KVHashTable* ht)
{
	if (!HashTable_NeedExpand(ht))
	{
		return;
	}

	int NumberOfNewSlots = ht->SlotCount + 1;
	int NumberOfNewBytes = sizeof(KVHashTableSlot) * NumberOfNewSlots;

	if ((ht->Expander)(NumberOfNewBytes, ht->ExpanderArg) == NULL)
	{
		return;
	}

	char* Dst = ((char*)(ht->EntryStart)) + NumberOfNewBytes;
	memmove(Dst, ht->EntryStart, ht->EntryCount_IncludingFreeAndStray * sizeof(KVHaskTableEntry));

	ht->SlotCount += NumberOfNewSlots;
	ht->EntryStart = (KVHaskTableEntry*)Dst;

	ResetSlots(ht);
	ht->DataCount = 0;

	KVHaskTableEntry* e;
	for (e = ht->EntryStart; e < ht->EntryStart + ht->EntryCount_IncludingFreeAndStray; ++e)
	{
		if (!(e->IsInFreeList))
		{
			KVHashTable_InsertInner(ht, e, e->Hash, 0 == 0);
		}
	}
}

void KVHashTable_GiveBackEntry(KVHashTable* ht, KVHaskTableEntry* entry)
{
	if (EntryIndex(ht, entry) != ht->EntryCount_IncludingFreeAndStray - 1)
	{
		return;
	}

	(ht->Expander)((-1) * (int)sizeof(KVHaskTableEntry), ht->ExpanderArg);
	--(ht->EntryCount_IncludingFreeAndStray);
}

void KVHashTable_Free(KVHashTable* List)
{

}