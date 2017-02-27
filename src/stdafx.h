#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <wincrypt.h>

#endif

#pragma pack(push, cryptoki, 1)
#include "./pkcs11.h"
#pragma pack(pop, cryptoki)

#include <stdio.h>
#include <memory>
#include <vector>

template <typename T>
using Scoped = std::shared_ptr<T>;

void SET_STRING(CK_UTF8CHAR* storage, char* data, int size);

// check incoming argument, if argument is NULL returns CKR_ARGUMENTS_BAD
#define CHECK_ARGUMENT_NULL(name)				\
	if (name == NULL_PTR) {						\
		return CKR_ARGUMENTS_BAD;				\
	}
