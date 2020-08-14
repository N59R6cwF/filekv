#include <string.h>
#include <Shlwapi.h>
#include "kvfilemapping.h"

/* private */ int DoMapFile(KVFileMapping* IndexMapping, unsigned int MinFileSize)
{
	if (IndexMapping->FileIsNewlyCreated)
	{
		if (MinFileSize == 0)
		{
			return -12;
		}
		else
		{
			IndexMapping->MappingSize = MinFileSize;
		}
	}
	else
	{
		DWORD FileSizeHigh = 0;
		DWORD FileSize = GetFileSize(IndexMapping->FileHandle, &FileSizeHigh);
		if (FileSizeHigh > 0)
		{
			// Not support too large file
			return -26;
		}

		if (MinFileSize == 0)
		{
			IndexMapping->MappingSize = FileSize;
		}
		else
		{
			if (MinFileSize < FileSize)
			{
				IndexMapping->MappingSize = FileSize;
			}
			else
			{
				IndexMapping->MappingSize = MinFileSize;
			}
		}
	}

	IndexMapping->MappingHandle = CreateFileMapping(
		IndexMapping->FileHandle,
		NULL,
		PAGE_READWRITE,
		0, IndexMapping->MappingSize,
		NULL
	);

	if (IndexMapping->MappingHandle == NULL || IndexMapping->MappingHandle == INVALID_HANDLE_VALUE)
	{
		return -57;
	}

	IndexMapping->MappingStart = MapViewOfFile(
		IndexMapping->MappingHandle,
		FILE_MAP_ALL_ACCESS,
		0, 0,
		0
	);

	if (IndexMapping->MappingStart == NULL)
	{
		return -69;
	}

	return 0;
}

KVFileMapping* NewKVFileMapping(const _TCHAR* FilePath, unsigned int MinFileSize)
{
	KVFileMapping* New = (KVFileMapping*)malloc(sizeof(KVFileMapping));
	if (New == NULL)
	{
		return NULL;
	}

	New->FileHandle = INVALID_HANDLE_VALUE;
	New->MappingHandle = INVALID_HANDLE_VALUE;
	New->MappingStart = NULL;

	New->FileHandle = CreateFile(
		FilePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (New->FileHandle == INVALID_HANDLE_VALUE)
	{
		KVFileMapping_Free(New);
		return NULL;
	}

	New->FileIsNewlyCreated = GetLastError() == 0;

	if (DoMapFile(New, MinFileSize) != 0) {
		KVFileMapping_Free(New);
		return NULL;
	}

	return New;
}

/* private */ void UnMapFile(KVFileMapping* IndexMapping)
{
	if (IndexMapping->MappingStart != NULL)
	{
		KVFileMapping_Flush(IndexMapping);
		UnmapViewOfFile(IndexMapping->MappingStart);
	}

	if (IndexMapping->MappingHandle != NULL && IndexMapping->MappingHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(IndexMapping->MappingHandle);
	}
}

int KVFileMapping_Expand(KVFileMapping* IndexMapping, unsigned int ExpandBy)
{
	if (UINT_MAX - IndexMapping->MappingSize < ExpandBy)
	{
		return -117;
	}

	IndexMapping->FileIsNewlyCreated = !!0;

	UnMapFile(IndexMapping);
	DoMapFile(IndexMapping, IndexMapping->MappingSize + ExpandBy);

	return 0;
}

void KVFileMapping_Flush(KVFileMapping* IndexMapping)
{
	FlushViewOfFile(IndexMapping->MappingStart, 0);
}

void KVFileMapping_Free(KVFileMapping* IndexMapping)
{
	if (IndexMapping == NULL)
	{
		return;
	}

	UnMapFile(IndexMapping);

	if (IndexMapping->FileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(IndexMapping->FileHandle);
	}

	free(IndexMapping);
}
