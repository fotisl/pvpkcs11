#include "public_key.h"

CK_RV PublicKey::GetAttributeValue
(
	CK_ATTRIBUTE_PTR  pTemplate,  /* specifies attributes; gets values */
	CK_ULONG          ulCount     /* attributes in template */
)
{
	CHECK_ARGUMENT_NULL(pTemplate);
	CK_RV res = CKR_OK;

	for (size_t i = 0; i < ulCount && res == CKR_OK; i++) {
		CK_ATTRIBUTE_PTR attr = &pTemplate[i];

		switch (attr->type) {
		case CKA_SUBJECT:
			res = GetSubject((CK_BYTE_PTR)attr->pValue, &attr->ulValueLen);
			break;
		case CKA_ENCRYPT:
			res = GetEncrypt((CK_BYTE_PTR)attr->pValue, &attr->ulValueLen);
			break;
		case CKA_VERIFY:
			res = GetVerify((CK_BYTE_PTR)attr->pValue, &attr->ulValueLen);
			break;
		case CKA_VERIFY_RECOVER:
			res = GetVerifyRecover((CK_BYTE_PTR)attr->pValue, &attr->ulValueLen);
			break;
		case CKA_WRAP:
			res = GetWrap((CK_BYTE_PTR)attr->pValue, &attr->ulValueLen);
			break;
		case CKA_TRUSTED:
			res = GetTrusted((CK_BYTE_PTR)attr->pValue, &attr->ulValueLen);
			break;
		case CKA_WRAP_TEMPLATE:
			res = GetWrapTemplate((CK_BYTE_PTR)attr->pValue, &attr->ulValueLen);
			break;
		default:
			res = Key::GetAttributeValue(attr, 1);
		}
	}

	return res;
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetClass) {
	return this->GetNumber(pValue, pulValueLen, CKO_PUBLIC_KEY);
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetSubject)
{
	return CKR_ATTRIBUTE_TYPE_INVALID;
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetEncrypt)
{
	return CKR_ATTRIBUTE_TYPE_INVALID;
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetVerify)
{
	return CKR_ATTRIBUTE_TYPE_INVALID;
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetVerifyRecover)
{
	return CKR_ATTRIBUTE_TYPE_INVALID;
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetWrap)
{
	return CKR_ATTRIBUTE_TYPE_INVALID;
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetTrusted)
{
	return CKR_ATTRIBUTE_TYPE_INVALID;
}

DECLARE_GET_ATTRIBUTE(PublicKey::GetWrapTemplate)
{
	return CKR_ATTRIBUTE_TYPE_INVALID;
}