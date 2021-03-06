#include "helper.h"

std::string GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = GetLastError();
    fprintf(stdout, "Error No: %u\n", errorMessageID);
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

typedef struct _NT_STATUS_MESSAGE {
    ULONG ulStatus;
    PCHAR pMessage;
} NT_STATUS_MESSAGE;

NT_STATUS_MESSAGE NT_STATUS_MESSAGES[] = {
    { 0, "The operation completed successfully." },
    { STATUS_INVALID_HANDLE, "An invalid HANDLE was specified" },
    { STATUS_BUFFER_TOO_SMALL, "The buffer is too small to contain the entry" },
    { STATUS_INVALID_PARAMETER, "One or more parameters are not valid." },
    { STATUS_NO_MEMORY, "A memory allocation failure occurred." },
    { STATUS_NOT_SUPPORTED, "The request is not supported." },
    { STATUS_DATA_ERROR, "An error occurred in reading or writing data." },
    { STATUS_NOT_FOUND, "The object was not found" },
    { NTE_INVALID_HANDLE, "The supplied handle is invalid." },
    { NTE_BAD_ALGID, "Wrong algorithm identity." },
    { NTE_BAD_FLAGS, "Wrong flags value." },
    { NTE_INVALID_PARAMETER, "One or more parameters are not valid." },
    { NTE_NO_MEMORY, "A memory allocation failure occurred." },
    { NTE_NOT_SUPPORTED, "The specified property is not supported for the object." },
    { NTE_PERM, "Access denied." },
    { NTE_NO_MORE_ITEMS, "The end of the enumeration has been reached." },
    { NTE_SILENT_CONTEXT, "The dwFlags parameter contains the NCRYPT_SILENT_FLAG flag, but the key being enumerated requires user interaction." },
    { NTE_BAD_TYPE, "Invalid type specified." }
};

std::string GetNTErrorAsString(NTSTATUS status)
{
    std::string hexCode("");
    hexCode.resize(sizeof(status) * 2);
    sprintf((PCHAR)hexCode.c_str(), "%X", status);
    ULONG ulMessagesCount = sizeof(NT_STATUS_MESSAGES) / sizeof(NT_STATUS_MESSAGE);
    std::string message("Unknown message");
    for (int i = 0; i < ulMessagesCount; i++) {
        if (NT_STATUS_MESSAGES[i].ulStatus == status) {
            message = std::string(NT_STATUS_MESSAGES[i].pMessage);
            break;
        }
    }
    return "(" + hexCode + ") " + message;
}