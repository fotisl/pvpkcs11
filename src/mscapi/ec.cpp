#include "ec.h"

using namespace mscapi;

Scoped<CryptoKeyPair> EcKey::Generate(
    CK_MECHANISM_PTR       pMechanism,                  /* key-gen mechanism */
    Scoped<core::Template> publicTemplate,
    Scoped<core::Template> privateTemplate
)
{
    try {
        if (pMechanism == NULL_PTR) {
            THROW_PKCS11_EXCEPTION(CKR_ARGUMENTS_BAD, "pMechanism is NULL");
        }
        if (pMechanism->mechanism != CKM_EC_KEY_PAIR_GEN) {
            THROW_PKCS11_MECHANISM_INVALID();
        }

        NTSTATUS status;
        auto point = publicTemplate->GetBytes(CKA_EC_PARAMS, true, "");

        Scoped<EcPrivateKey> privateKey(new EcPrivateKey());
        privateKey->GenerateValues(privateTemplate->Get(), privateTemplate->Size());

        Scoped<EcPublicKey> publicKey(new EcPublicKey());
        publicKey->GenerateValues(publicTemplate->Get(), publicTemplate->Size());

        LPCWSTR pszAlgorithm;

#define POINT_COMPARE(curve) memcmp(core::EC_##curve##_BLOB, point->data(), sizeof(core::EC_##curve##_BLOB)-1 ) == 0

        if (POINT_COMPARE(P256)) {
            pszAlgorithm = NCRYPT_ECDSA_P256_ALGORITHM;
        }
        else if (POINT_COMPARE(P384)) {
            pszAlgorithm = NCRYPT_ECDSA_P384_ALGORITHM;
        }
        else if (POINT_COMPARE(P521)) {
            pszAlgorithm = NCRYPT_ECDSA_P521_ALGORITHM;
        }
        else {
            THROW_PKCS11_EXCEPTION(CKR_TEMPLATE_INCOMPLETE, "Wrong POINT for EC key");
        }

#undef POINT_COMPARE

        // NCRYPT
        Scoped<ncrypt::Provider> provider(new ncrypt::Provider());
        provider->Open(MS_KEY_STORAGE_PROVIDER, 0);

        Scoped<ncrypt::Key> key;
        if (!privateKey->ItemByType(CKA_TOKEN)->To<core::AttributeBool>()->ToValue()) {
            key = provider->CreatePersistedKey(pszAlgorithm, NULL, 0, 0);
        }
        else {
            key = provider->CreatePersistedKey(pszAlgorithm, provider->GenerateRandomName()->c_str(), 0, 0);
        }

        // Key Usage
        ULONG keyUsage = 0;
        if (publicTemplate->GetBool(CKA_SIGN, false, false) || publicTemplate->GetBool(CKA_VERIFY, false, false)) {
            keyUsage |= NCRYPT_ALLOW_SIGNING_FLAG;
        }
        if (publicTemplate->GetBool(CKA_DERIVE, false, false)) {
            keyUsage |= NCRYPT_ALLOW_KEY_AGREEMENT_FLAG;
        }
        key->SetNumber(NCRYPT_KEY_USAGE_PROPERTY, keyUsage);

        auto attrToken = privateKey->ItemByType(CKA_TOKEN)->To<core::AttributeBool>()->ToValue();
        auto attrExtractable = privateKey->ItemByType(CKA_EXTRACTABLE)->To<core::AttributeBool>()->ToValue();
        if ((attrToken && attrExtractable) || !attrToken) {
            // Make all session keys extractable. It allows to copy keys from session to storage via export/import
            // This is extractable only for internal usage. Key object will have CKA_EXTRACTABLE with setted value
            key->SetNumber(NCRYPT_EXPORT_POLICY_PROPERTY, NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG, NCRYPT_PERSIST_FLAG);
        }

        key->Finalize();

        privateKey->Assign(key);
        publicKey->Assign(key);

        return Scoped<CryptoKeyPair>(new CryptoKeyPair(privateKey, publicKey));
    }
    CATCH_EXCEPTION;
}

void EcPrivateKey::FillPublicKeyStruct()
{
    try {
        auto buffer = nkey->ExportKey(BCRYPT_ECCPUBLIC_BLOB, 0);
        PUCHAR pbKey = buffer->data();
        BCRYPT_ECCKEY_BLOB* header = (BCRYPT_ECCKEY_BLOB*)pbKey;

        // CKA_PARAM
        switch (header->dwMagic) {
        case BCRYPT_ECDH_PUBLIC_P256_MAGIC:
        case BCRYPT_ECDSA_PUBLIC_P256_MAGIC:
            ItemByType(CKA_EC_PARAMS)->SetValue((CK_VOID_PTR)&core::EC_P256_BLOB, 10);
            break;
        case BCRYPT_ECDH_PUBLIC_P384_MAGIC:
        case BCRYPT_ECDSA_PUBLIC_P384_MAGIC:
            ItemByType(CKA_EC_PARAMS)->SetValue((CK_VOID_PTR)&core::EC_P384_BLOB, 7);
            break;
        case BCRYPT_ECDH_PUBLIC_P521_MAGIC:
        case BCRYPT_ECDSA_PUBLIC_P521_MAGIC: {
            ItemByType(CKA_EC_PARAMS)->SetValue((CK_VOID_PTR)&core::EC_P521_BLOB, 7);
            break;
        }
        default:
            THROW_PKCS11_EXCEPTION(CKR_FUNCTION_FAILED, "Unsupported named curve");
        }

        DWORD keyUsage = NCRYPT_ALLOW_SIGNING_FLAG | NCRYPT_ALLOW_KEY_AGREEMENT_FLAG;
        // NCRYPT_KEY_USAGE_PROPERTY can contain zero or a combination of one or more of the values
        try {
            keyUsage = nkey->GetNumber(NCRYPT_KEY_USAGE_PROPERTY);
        }
        catch (...) {
            // Cannot get NCRYPT_KEY_USAGE_PROPERTY
        }
        if (keyUsage & NCRYPT_ALLOW_SIGNING_FLAG) {
            ItemByType(CKA_SIGN)->To<core::AttributeBool>()->Set(true);
        }
        if (keyUsage & NCRYPT_ALLOW_KEY_AGREEMENT_FLAG) {
            ItemByType(CKA_DERIVE)->To<core::AttributeBool>()->Set(true);
        }

    }
    CATCH_EXCEPTION
}

void EcPrivateKey::FillPrivateKeyStruct()
{
    try {
        auto buffer = nkey->ExportKey(BCRYPT_ECCPUBLIC_BLOB, 0);
        PUCHAR pbKey = buffer->data();
        BCRYPT_ECCKEY_BLOB* header = (BCRYPT_ECCKEY_BLOB*)pbKey;
        PCHAR pValue = (PCHAR)(pbKey + sizeof(BCRYPT_ECCKEY_BLOB) + (header->cbKey * 2));

        // CK_VALUE
        ItemByType(CKA_VALUE)->SetValue(pValue, header->cbKey);
    }
    CATCH_EXCEPTION
}

CK_RV EcPrivateKey::GetValue(
    CK_ATTRIBUTE_PTR attr
)
{
    try {
        core::EcPrivateKey::GetValue(attr);

        switch (attr->type) {
        case CKA_EC_PARAMS:
            if (ItemByType(attr->type)->IsEmpty()) {
                FillPublicKeyStruct();
            }
            break;
        case CKA_VALUE:
            if (ItemByType(attr->type)->IsEmpty()) {
                FillPrivateKeyStruct();
            }
            break;
        }

        return CKR_OK;
    }
    CATCH_EXCEPTION
}

CK_RV EcPrivateKey::CopyValues(
    Scoped<core::Object>    object,     /* the object which must be copied */
    CK_ATTRIBUTE_PTR        pTemplate,  /* specifies attributes */
    CK_ULONG                ulCount     /* attributes in template */
)
{
    try {
        core::EcPrivateKey::CopyValues(
            object,
            pTemplate,
            ulCount
        );

        EcPrivateKey* originalKey = dynamic_cast<EcPrivateKey*>(object.get());
        if (!originalKey) {
            THROW_PKCS11_EXCEPTION(CKR_FUNCTION_FAILED, "Original key must be RsaPrivateKey");
        }

        ncrypt::Provider provider;
        provider.Open(MS_KEY_STORAGE_PROVIDER, 0);

        auto attrToken = ItemByType(CKA_TOKEN)->To<core::AttributeBool>()->ToValue();
        auto attrExtractable = ItemByType(CKA_EXTRACTABLE)->To<core::AttributeBool>()->ToValue();

        nkey = ncrypt::CopyKeyToProvider(
            originalKey->nkey.get(),
            BCRYPT_ECCPRIVATE_BLOB,
            &provider,
            attrToken ? provider.GenerateRandomName()->c_str() : NULL,
            (attrToken && attrExtractable) || !attrToken
        );

        return CKR_OK;
    }
    CATCH_EXCEPTION
}

CK_RV mscapi::EcPrivateKey::Destroy()
{
    try {
        nkey->Delete(0);

        return CKR_OK;
    }
    CATCH_EXCEPTION
}

void EcPrivateKey::OnKeyAssigned()
{
    try {
        FillPublicKeyStruct();
    }
    CATCH_EXCEPTION
}

// Public key

void EcPublicKey::FillKeyStruct()
{
    try {
        auto buffer = nkey->ExportKey(BCRYPT_ECCPUBLIC_BLOB, 0);
        PUCHAR pbKey = buffer->data();
        BCRYPT_ECCKEY_BLOB* header = (BCRYPT_ECCKEY_BLOB*)pbKey;
        PCHAR pPoint = (PCHAR)(pbKey + sizeof(BCRYPT_ECCKEY_BLOB));

        // POINT
        auto propPoint = Scoped<std::string>(new std::string(""));

        // PARAM
        switch (header->dwMagic) {
        case BCRYPT_ECDH_PUBLIC_P256_MAGIC:
        case BCRYPT_ECDSA_PUBLIC_P256_MAGIC:
            ItemByType(CKA_EC_PARAMS)->SetValue((CK_VOID_PTR)core::EC_P256_BLOB, 10);
            *propPoint += std::string("\x04\x41\x04");
            break;
        case BCRYPT_ECDH_PUBLIC_P384_MAGIC:
        case BCRYPT_ECDSA_PUBLIC_P384_MAGIC:
            ItemByType(CKA_EC_PARAMS)->SetValue((CK_VOID_PTR)core::EC_P384_BLOB, 7);
            *propPoint += std::string("\x04\x61\x04");
            break;
        case BCRYPT_ECDH_PUBLIC_P521_MAGIC:
        case BCRYPT_ECDSA_PUBLIC_P521_MAGIC: {
            ItemByType(CKA_EC_PARAMS)->SetValue((CK_VOID_PTR)core::EC_P521_BLOB, 7);
            *propPoint += std::string("\x04\x81\x85\x04");
            break;
        }
        default:
            THROW_PKCS11_EXCEPTION(CKR_FUNCTION_FAILED, "Unsupported named curve");
        }
        *propPoint += std::string(pPoint, header->cbKey * 2);
        ItemByType(CKA_EC_POINT)->SetValue((CK_VOID_PTR)propPoint->c_str(), propPoint->length());

        auto keyUsage = nkey->GetNumber(NCRYPT_KEY_USAGE_PROPERTY);
        if (keyUsage & NCRYPT_ALLOW_SIGNING_FLAG) {
            ItemByType(CKA_VERIFY)->To<core::AttributeBool>()->Set(true);
        }
        if (keyUsage & NCRYPT_ALLOW_KEY_AGREEMENT_FLAG) {
            ItemByType(CKA_DERIVE)->To<core::AttributeBool>()->Set(true);
        }

    }
    CATCH_EXCEPTION;
}

CK_RV EcPublicKey::GetValue(
    CK_ATTRIBUTE_PTR attr
)
{
    try {
        core::EcPublicKey::GetValue(attr);

        switch (attr->type) {
        case CKA_EC_PARAMS:
        case CKA_EC_POINT:
            if (ItemByType(attr->type)->IsEmpty()) {
                FillKeyStruct();
            }
            break;
        }

        return CKR_OK;
    }
    CATCH_EXCEPTION
}

Scoped<core::Object> EcKey::DeriveKey(
    CK_MECHANISM_PTR        pMechanism,
    Scoped<core::Object>    baseKey,
    Scoped<core::Template>  tmpl
)
{
    try {
        EcPrivateKey* ecPrivateKey = dynamic_cast<EcPrivateKey*>(baseKey.get());
        if (!ecPrivateKey) {
            THROW_PKCS11_EXCEPTION(CKR_KEY_TYPE_INCONSISTENT, "baseKey is not EC key");
        }

        if (pMechanism == NULL_PTR) {
            THROW_PKCS11_EXCEPTION(CKR_ARGUMENTS_BAD, "pMechanism is NULL");
        }
        if (pMechanism->mechanism != CKM_ECDH1_DERIVE) {
            THROW_PKCS11_EXCEPTION(CKR_MECHANISM_INVALID, "pMechanism->mechanism is not CKM_ECDH1_DERIVE");
        }
        if (pMechanism->pParameter == NULL_PTR) {
            THROW_PKCS11_EXCEPTION(CKR_MECHANISM_PARAM_INVALID, "pMechanism->pParameter is NULL");
        }
        CK_ECDH1_DERIVE_PARAMS_PTR params = static_cast<CK_ECDH1_DERIVE_PARAMS_PTR>(pMechanism->pParameter);
        if (!params) {
            THROW_PKCS11_EXCEPTION(CKR_MECHANISM_PARAM_INVALID, "pMechanism->pParameter is not CK_ECDH1_DERIVE_PARAMS");
        }

        THROW_PKCS11_FUNCTION_NOT_SUPPORTED();
    }
    CATCH_EXCEPTION
}

CK_RV EcPublicKey::CreateValues
(
    CK_ATTRIBUTE_PTR  pTemplate,  /* specifies attributes */
    CK_ULONG          ulCount     /* attributes in template */
)
{
    try {
        core::Template tmpl(pTemplate, ulCount);
        core::EcPublicKey::CreateValues(pTemplate, ulCount);

        NTSTATUS status;
        Scoped<Buffer> buffer(new Buffer);


        // Named curve
        auto params = ItemByType(CKA_EC_PARAMS)->To<core::AttributeBytes>()->ToValue();

        ULONG dwMagic;
        ULONG keySize;
        if (!memcmp(core::EC_P256_BLOB, params->data(), sizeof(core::EC_P256_BLOB) - 1)) {
            dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
            keySize = 32;
        }
        else if (!memcmp(core::EC_P384_BLOB, params->data(), sizeof(core::EC_P384_BLOB) - 1)) {
            dwMagic = BCRYPT_ECDSA_PUBLIC_P384_MAGIC;
            keySize = 48;
        }
        else if (!memcmp(core::EC_P521_BLOB, params->data(), sizeof(core::EC_P521_BLOB) - 1)) {
            dwMagic = BCRYPT_ECDSA_PUBLIC_P521_MAGIC;
            keySize = 66;
        }
        else {
            THROW_PKCS11_EXCEPTION(CKR_ATTRIBUTE_VALUE_INVALID, "Wrong NamedCorve value");
        }

        auto tmplPoint = tmpl.GetBytes(CKA_EC_POINT, true, "");
        auto decodedPoint = core::EcUtils::DecodePoint(tmplPoint, keySize);

        buffer->resize(sizeof(BCRYPT_ECCKEY_BLOB));
        BCRYPT_ECCKEY_BLOB* header = (BCRYPT_ECCKEY_BLOB*)buffer->data();
        header->dwMagic = dwMagic;
        header->cbKey = keySize;
        buffer->insert(buffer->end(), decodedPoint->X->begin(), decodedPoint->X->end());
        buffer->insert(buffer->end(), decodedPoint->Y->begin(), decodedPoint->Y->end());

        ncrypt::Provider provider;
        provider.Open(NULL, 0);

        auto key = provider.ImportKey(BCRYPT_ECCPUBLIC_BLOB, buffer->data(), buffer->size(), 0);
        Assign(key);

        return CKR_OK;
    }
    CATCH_EXCEPTION
}

CK_RV EcPublicKey::CopyValues(
    Scoped<core::Object>    object,     /* the object which must be copied */
    CK_ATTRIBUTE_PTR        pTemplate,  /* specifies attributes */
    CK_ULONG                ulCount     /* attributes in template */
)
{
    try {
        core::EcPublicKey::CopyValues(
            object,
            pTemplate,
            ulCount
        );

        EcPublicKey* originalKey = dynamic_cast<EcPublicKey*>(object.get());
        if (!originalKey) {
            THROW_PKCS11_EXCEPTION(CKR_FUNCTION_FAILED, "Original key must be EcPrivateKey");
        }

        // It'll not be added to storage. Because mscapi slot creates 2 keys (private/public) from 1 key container

        nkey = originalKey->nkey;

        return CKR_OK;
    }
    CATCH_EXCEPTION
}

CK_RV mscapi::EcPublicKey::Destroy()
{
    try {
        return CKR_OK;
    }
    CATCH_EXCEPTION
}

void EcPublicKey::OnKeyAssigned()
{
    try {
        FillKeyStruct();
    }
    CATCH_EXCEPTION
}