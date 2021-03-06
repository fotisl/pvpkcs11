#pragma once

#include "../stdafx.h"
#include <CoreFoundation/CoreFoundation.h>


namespace osx {
    
    std::string GetOSXErrorAsString(OSStatus status, const char* funcName);
    
#define OSX_EXCEPTION_NAME "OSXException"
    
#define THROW_OSX_EXCEPTION(status, funcName)                                        \
throw Scoped<core::Exception>(new core::Pkcs11Exception(OSX_EXCEPTION_NAME, CKR_FUNCTION_FAILED, GetOSXErrorAsString(status, funcName).c_str(), __FUNCTION__, __FILE__, __LINE__))
    
    template<typename T>
    class CFRef {
    public:
        CFRef() : value(NULL), free(CFRelease) {
//            fprintf(stdout, "%s\n", __FUNCTION__, typeid(value).name());
        }
        
        CFRef(T value) : value(value), free(CFRelease) {
//            fprintf(stdout, "%s:%s %p\n", __FUNCTION__, typeid(value).name(), value);
        }
        
        CFRef(T value, void (*free)(const void* ref)) : value(value), free(free) {
//            fprintf(stdout, "%s:$s %p %p\n", __FUNCTION__, value, typeid(value).name(), free);
        }
        
        ~CFRef(){
            if (value && free) {
//                fprintf(stdout, "%s:%s\n", __FUNCTION__, typeid(value).name());
                free(value);
                value = NULL;
            }
        }
        
        T Get() {
            return value;
        }
        
        CFRef<T>& operator=(const T data) {
            value = data;
            return *this;
        }
        
        T operator&() {
            return value;
        }
        
        bool IsEmpty() {
            return value == NULL;
        }
        
    protected:
        T value;
        void (*free)(const void* ref);
    };
    
    static CFStringRef kSecAttrLabelModule = (CFSTR("WebCrypto Local"));

}
