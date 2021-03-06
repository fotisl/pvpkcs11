#include "../bcrypt.h"

using namespace bcrypt;

Key::Key(
    BCRYPT_KEY_HANDLE handle
)
{
    this->handle = handle;
}

Key::~Key()
{
    Destroy();
}

void Key::Destroy()
{
    if (handle) {
        BCryptDestroyKey(handle);
        handle = NULL;
    }
}

void Key::Finalize(
    _In_    ULONG   dwFlags
)
{
    NTSTATUS status = BCryptFinalizeKeyPair(handle, dwFlags);
    if (status) {
        THROW_NT_EXCEPTION(status);
    }
}

Scoped<Key> Key::Duplicate()
{
    BCRYPT_KEY_HANDLE hKeyCopy;
    NTSTATUS status = BCryptDuplicateKey(handle, &hKeyCopy, NULL, 0, 0);
    if (status) {
        THROW_NT_EXCEPTION(status);
    }

    return Scoped<Key>(new Key(hKeyCopy));
}

Scoped<Algorithm> Key::GetProvider()
{
	try {
		BCRYPT_ALG_HANDLE hAlg;
		ULONG ulHandleLen;
		NTSTATUS status = BCryptGetProperty(handle, BCRYPT_PROVIDER_HANDLE, NULL, 0, &ulHandleLen, 0);
		if (status) {
			THROW_NT_EXCEPTION(status);
		}
		hAlg = (BCRYPT_ALG_HANDLE)malloc(ulHandleLen);
		status = BCryptGetProperty(handle, BCRYPT_PROVIDER_HANDLE, (PUCHAR)hAlg, ulHandleLen, &ulHandleLen, 0);
		if (status) {
			THROW_NT_EXCEPTION(status);
		}
		return Scoped<Algorithm>(new Algorithm(hAlg));
	}
	CATCH_EXCEPTION
}