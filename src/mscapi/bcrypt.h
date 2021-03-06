#pragma once

#include "../stdafx.h"

#include <bcrypt.h>
#include "helper.h"

namespace bcrypt {

	template<typename T>
	class Object {
	public:
		Object() {}
		Object(T handle) : handle(handle) {};

		void Set(T handle) {
			this->handle = handle;
		}

		T Get() {
			return handle;
		}

		void GetParam(LPCWSTR pszPropId, BYTE* pbData, DWORD* pdwDataLen, DWORD dwFlags = 0)
		{
			NTSTATUS status;
			if (!pbData) {
				status = BCryptGetProperty(handle, pszPropId, NULL, 0, pdwDataLen, dwFlags);
			}
			else {
				status = BCryptGetProperty(handle, pszPropId, pbData, *pdwDataLen, pdwDataLen, dwFlags);
			}
			if (status) {
				THROW_NT_EXCEPTION(status);
			}
		}

		DWORD GetNumber(LPCWSTR pszPropId)
		{
			try {
				DWORD dwData;
				DWORD dwDataLen = sizeof(dwData);

				GetParam(pszPropId, (BYTE*)&dwData, &dwDataLen);

				return dwData;
			}
			CATCH_EXCEPTION;
		}

		bool GetBoolean(LPCWSTR pszPropId)
		{
			try {
				bool bData;
				DWORD dwDataLen = sizeof(bData);

				GetParam(pszPropId, (BYTE*)&bData, &dwDataLen);

				return bData;
			}
			CATCH_EXCEPTION;
		}

		Scoped<std::string> GetBytes(LPCWSTR pszPropId)
		{
			try {
				Scoped<std::string> strData(new std::string(""));
				DWORD dwDataLen = sizeof(strData);

				GetParam(pszPropId, NULL, &dwDataLen);
				strData->resize(dwDataLen);
				GetParam(pszPropId, (BYTE*)strData->c_str(), &dwDataLen);

				return strData;
			}
			CATCH_EXCEPTION;
		}

		Scoped<std::wstring> GetBytesW(LPCWSTR pszPropId)
		{
			try {
				auto data = GetBytes(pszPropId);

				return Scoped<std::wstring>(new std::wstring((wchar_t*)data->c_str()));
			}
			CATCH_EXCEPTION;
		}

		void SetParam(
			_In_                    LPCWSTR pszProperty,
			_In_reads_bytes_(cbInput)    PUCHAR   pbInput,
			_In_                    ULONG   cbInput,
			_In_                    ULONG   dwFlags = 0
		)
		{
			NTSTATUS status = BCryptSetProperty(
				handle,
				pszProperty,
				pbInput,
				cbInput,
				dwFlags
			);
			if (status) {
				THROW_NT_EXCEPTION(status);
			}
		}

		void SetNumber(
			LPCWSTR pszProperty,
			ULONG   ulValue,
			ULONG   dwFlags = 0
		)
		{
			try {
				SetParam(
					pszProperty,
					ulValue,
					sizeof(ulValue),
					dwFlags
				);
			}
			CATCH_EXCEPTION;
		}

		void SetBoolean(
			LPCWSTR pszProperty,
			bool    bbValue,
			ULONG   dwFlags = 0
		)
		{
			try {
				SetParam(
					pszProperty,
					bbValue,
					sizeof(bbValue),
					dwFlags
				);
			}
			CATCH_EXCEPTION;
		}

	protected:
		T handle;
	};

	class Algorithm;

	class Key : public Object<BCRYPT_KEY_HANDLE> {
	public:
		Key(
			BCRYPT_KEY_HANDLE handle
		);
		~Key();

		Scoped<Algorithm> GetProvider();

		void Destroy();
		void Finalize(
			_In_    ULONG   dwFlags
		);

		Scoped<Key> Duplicate();
	};

	class Algorithm : public Object<BCRYPT_ALG_HANDLE> {
	public:
		Algorithm() {}
		Algorithm(BCRYPT_ALG_HANDLE handle) { this->handle = handle; }
		~Algorithm();

		void Open(
			_In_        LPCWSTR              pszAlgId,
			_In_opt_    LPCWSTR              pszImplementation,
			_In_        ULONG                dwFlags
		);

		Scoped<std::string> GenerateRandom(
			_In_    ULONG   dwLength
		);

		Scoped<bcrypt::Key> Algorithm::GenerateKey(
			_Out_writes_bytes_all_opt_(cbKeyObject) PUCHAR  pbKeyObject,
			_In_                                    ULONG   cbKeyObject,
			_In_reads_bytes_(cbSecret)              PUCHAR  pbSecret,
			_In_                                    ULONG   cbSecret,
			_In_                                    ULONG   dwFlags
		);

        Scoped<Key> ImportKey(
            _In_        LPCWSTR             pszBlobType,
            _In_reads_bytes_(cbData) PBYTE  pbData,
            _In_        DWORD               cbData,
            _In_        DWORD               dwFlags
        );

	};

}