#include <stdio.h>

// Although this project includes tchar.h which is Windows-only, 
// this project doesn't adapt any Windows-only concept.
// It is just for convenience when dealing with file pathes
#include <tchar.h>

#include "..\\filekv.h"

// Hash function used for hashing keys
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

int ForeachCallback(const char* Key, const char* Val)
{
	printf("KEY: %s\nVAL: %s\n\n", Key, Val);
	return 0;
}

int main(void)
{
	// Open or create FileKV from file.
	FileKV* kv = NewFileKV(_T("x64\\kv.txt"), SimpleHash);

	// Add
	FileKV_Put(kv, "LjoqfuUI5Txaz", "iLsXu8iAM6cOOAAY");

	// Overwrite
	FileKV_Put(kv, "LjoqfuUI5Txaz", "yAmSECqYmnt3aMMC");

	FileKV_Put(kv, "hRGvah", "Zc6BtKfkBxqHkquIUGUhKkWB8STo64tb5cDY9yAygy");
	FileKV_Put(kv, "YIRsvgSvYqgEo", "tTiW6zEJ7bQCQi7fy6VLLE9");
	FileKV_Put(kv, "oYpFPCR7Dgu0OsBOwSPL", "BZNpNDTYHtS6rqYag0Ujba8Srqui9RrVi4F3rTTKhKw");
	FileKV_Put(kv, "hBtY6zVidY1NeK2quzf6", "buSbtI7LH6Pjk3yJu8mmSeCL5Kux77bNbrFXSsRVbsceEo");
	FileKV_Put(kv, "MMVzHEF", "28CBiJFPZNBpsyTBnQrQ6QrZEU1xQ0rUq35nNmdDPQ9aQ3");
	FileKV_Put(kv, "QBQpNx0kxmv3ihZ9kG00", "BL6gFj2PgeYMhNeoBq8tJ6Bo5GlwjmWTFaZEJQpLAyUFOn");
	FileKV_Put(kv, "6QraM9ypM65Bu", "ikkflxA8I2H1Rda4NZDucj5XxVkk7msvN7ji9kpcMzH");
	FileKV_Put(kv, "aSeHvqwv8vbUhH1K1zV8Ai", "4FpzAVRwPrkYaNUjn3oqOFb");
	FileKV_Put(kv, "GpFIm39s5I40sH5yGILPJR", "OQIPqKFtOfaozUArMEJL6z1");
	FileKV_Put(kv, "qeDeXMddPUWopPGdKZ3XmN", "R7AU2NSBxpVhqAj47aA7GGgsKQrrN4ZscFb3KVPiLb1");
	FileKV_Put(kv, "hErvJ3lVDk8NA34", "E49DD4VbGtxWD3yPjiygWqCJcTPkJ2v7IBoyy4sx6oJ");
	FileKV_Put(kv, "wOVg0fMwdcuGmGJ6ZhNP", "QtEKwCIQfznHuXwJvMJ6QJoe6R0QgKApYSqoAlLCoorMN");
	FileKV_Put(kv, "rpz898RFQFiCikqiZ6sT", "pEL6RTiiv4MtYzFbetKrsAjsxga67POKu9qB6mJfjhHcE");
	FileKV_Put(kv, "lqEhaj4YQL", "tvLGIMo461mQkufpvPsFWl0xBHe0Pp2ITz0BSsfu9e");
	FileKV_Put(kv, "57Pno1cuJfGwH0RNETJv4QS", "vJyi7sdUhRqJF2O4rHx7EsaOhbxUou2p6CddbmDGhzUuqu");
	FileKV_Put(kv, "uEIDA8a9zz38Eu8UOVzT5A8", "mqJZdG");
	FileKV_Put(kv, "u9YR1Ie1VdCklacFpgXHcX0", "7SqEi4pD60YR0d2PliP5AMNafxMsAsvY2rbkphwDNi41B8");
	FileKV_Put(kv, "vJASuG5DeawUL", "e5Pm1iD5UlAEOf2Tfkd0Tt1k3tiCqv87cQhTtIQjCPLGTb");
	FileKV_Put(kv, "wi1JZLogLGGgVSuOwxkTCDv", "ofW7WAbkx18NYhGo1B4gwRaGKURQmpy8DlovfkW2YQJw");
	FileKV_Put(kv, "mz5vzIYYGHogjyuvoRS8qQL", "nKh5OFBQIUxpw7VqiYhRxEgxwWKeonjm68RmIGc2E42G");

	// Remove a key and it's value
	FileKV_Remove(kv, "hRGvah");

	// Print the number of key-value pairs.
	printf("Count: %d\n\n", FileKV_Count(kv));

	// Print all key-value pairs.
	FileKV_Foreach(kv, ForeachCallback);

	// Free resources. This won't delete files.
	FileKV_Free(kv);

	return 0;
}
