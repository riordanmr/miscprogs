/* jsstub.c: program to use as a stub for a Windows EXE file that executes a JavaScript program.
   Use this program by copying JavaScript code to the end of the EXE file, then run the EXE.
   Example:
      cl /EHsc jsstub.c /link ole32.lib oleaut32.lib 
      copy /b jsstub.exe+dirlist.js dirlist.exe
   Then you can execute dirlist.exe.

   The purpose of this program is to investigate the possibility of doing the same
   for the Icon interpreter.

   Mark Riordan   2025-03-16
   Credits to https://stackoverflow.com/questions/34684660/how-to-determine-the-size-of-an-pe-executable-file-from-headers-and-or-footers
 */ 

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>
#include <activscp.h>
#include <initguid.h>

#define BAD_EXE 0xfffffffe

/* Find the end of the original PE portion of an executable file.
 * Entry: buffer: pointer to the start of the executable file in memory
 *        bufsize: size of the buffer in bytes  
 * Exit:  Returns the offset of the end of the original PE portion of the file.
 *          Normally this would be equal to bufsize, but would be less than 
 *          bufsize if data has been appended to the executable.
 *        Returns BAD_EXE if the file is not a valid PE executable.
 */
size_t FindPayloadOffset(const unsigned char * buffer, size_t bufsize)
{
   size_t offset=0;

    if (bufsize < sizeof(IMAGE_DOS_HEADER)) {
        fprintf(stderr, "Buffer size is too small for IMAGE_DOS_HEADER\n");
        return BAD_EXE;
    }
  
    IMAGE_DOS_HEADER* dosheader = (IMAGE_DOS_HEADER*)buffer;
    
    if (dosheader->e_magic != IMAGE_DOS_SIGNATURE) {
        fprintf(stderr, "Invalid DOS signature\n");
        return BAD_EXE;
    }
    
    if (bufsize < (size_t)(dosheader->e_lfanew) + sizeof(IMAGE_NT_HEADERS)) {
        fprintf(stderr, "Buffer size is too small for IMAGE_NT_HEADERS\n");
        return BAD_EXE;
    }
    
    IMAGE_NT_HEADERS* ntheader = (IMAGE_NT_HEADERS*)(buffer + dosheader->e_lfanew);
    
    if (ntheader->Signature != IMAGE_NT_SIGNATURE) {
        fprintf(stderr, "Invalid NT signature\n");
        return BAD_EXE;
    }

    if (bufsize < (size_t)(dosheader->e_lfanew) + sizeof(IMAGE_NT_HEADERS) +
       sizeof(IMAGE_SECTION_HEADER) * ntheader->FileHeader.NumberOfSections)
    {
        fprintf(stderr, "Buffer size is too small for IMAGE_SECTION_HEADER\n");
        return BAD_EXE;
    }

    IMAGE_SECTION_HEADER* sectiontable = (IMAGE_SECTION_HEADER*)((uint8_t*)ntheader + sizeof(IMAGE_NT_HEADERS));
    
    if (ntheader->FileHeader.PointerToSymbolTable)
    {
        if (0==strncmp((char*)sectiontable->Name, "UPX", 3)) {
            fprintf(stderr, "Unstripped UPX Compressed Executables are NOT SUPPORTED\n");
            // UPX produces exe with invalid PointerToSymbolTable
            return BAD_EXE;
        }
        
        IMAGE_SYMBOL* symboltable = (IMAGE_SYMBOL*)(buffer + ntheader->FileHeader.PointerToSymbolTable);
        IMAGE_SYMBOL* stringtable = symboltable + ntheader->FileHeader.NumberOfSymbols;
        int stringtable_len =  *(int32_t*)stringtable;
        
        uint8_t* end = (uint8_t*)stringtable + stringtable_len;
        offset = end - buffer;
    } else {
        /* stripped */
        IMAGE_SECTION_HEADER* last_section_header = sectiontable + ntheader->FileHeader.NumberOfSections - 1;
        offset = last_section_header->PointerToRawData + last_section_header->SizeOfRawData;
    }
    
    DWORD align = ntheader->OptionalHeader.FileAlignment;
    if (offset % align) offset = (offset/align + 1)*align;
    
    if (offset > bufsize) {
        fprintf(stderr, "Offset exceeds buffer size\n");
        return BAD_EXE;
    }

    return offset;
}

/* Load the entire contents of the current executable file into memory.
 * Exit:    size is the size of the file in bytes
 *          Returns a pointer to a buffer containing the file contents, or 
 *            NULL if an error occurs. The buffer is null-terminated.
 */
unsigned char* LoadCurrentExe(size_t* size) {
    // Get the path of the current executable
    char exe_path[MAX_PATH];
    if (GetModuleFileName(NULL, exe_path, MAX_PATH) == 0) {
        fprintf(stderr, "Error getting executable path\n");
        return NULL;
    }

    // Open the executable file
    FILE* file = fopen(exe_path, "rb");
    if (!file) {
        fprintf(stderr, "Error opening %s\n", exe_path);
        return NULL;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer to hold the file contents, plus one for null terminator
    unsigned char* buffer = (unsigned char*)malloc(1 + *size);
    if (!buffer) {
        fprintf(stderr, "Error allocating memory\n");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    if (fread(buffer, 1, *size, file) != *size) {
        fprintf(stderr, "Error reading file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Close the file
    fclose(file);

    buffer[*size] = '\0'; // Null-terminate the buffer

    return buffer;
}

// Define CLSID_JScript and IID_IActiveScript
DEFINE_GUID(CLSID_JScript, 0xf414c260, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58);
DEFINE_GUID(IID_IActiveScript, 0xbb1a2ae1, 0xa4f9, 0x11cf, 0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64);
DEFINE_GUID(CATID_ActiveScriptParse, 0xf0b7a1a2, 0x9847, 0x11cf, 0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64);

/*======================================================================
 * Implementation of the IActiveScriptSite interface, necessary to use 
 * Windows Scripting Engine for JavaScript execution.
 * This was mostly generated by GitHub Copilot.
 */
typedef struct {
    IActiveScriptSiteVtbl* lpVtbl;
    ULONG refCount;
} ScriptSite;

HRESULT STDMETHODCALLTYPE ScriptSite_QueryInterface(IActiveScriptSite* This, REFIID riid, void** ppvObject) {
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IActiveScriptSite)) {
        *ppvObject = This;
        This->lpVtbl->AddRef(This);
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ScriptSite_AddRef(IActiveScriptSite* This) {
    ScriptSite* site = (ScriptSite*)This;
    return InterlockedIncrement(&site->refCount);
}

ULONG STDMETHODCALLTYPE ScriptSite_Release(IActiveScriptSite* This) {
    ScriptSite* site = (ScriptSite*)This;
    ULONG refCount = InterlockedDecrement(&site->refCount);
    if (refCount == 0) {
        free(site);
    }
    return refCount;
}

HRESULT STDMETHODCALLTYPE ScriptSite_GetLCID(IActiveScriptSite* This, LCID* plcid) {
    *plcid = LOCALE_USER_DEFAULT;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScriptSite_GetItemInfo(IActiveScriptSite* This, LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti) {
    return TYPE_E_ELEMENTNOTFOUND;
}

HRESULT STDMETHODCALLTYPE ScriptSite_GetDocVersionString(IActiveScriptSite* This, BSTR* pbstrVersion) {
    *pbstrVersion = SysAllocString(L"1.0");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScriptSite_OnScriptTerminate(IActiveScriptSite* This, const VARIANT* pvarResult, const EXCEPINFO* pexcepinfo) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScriptSite_OnStateChange(IActiveScriptSite* This, SCRIPTSTATE ssScriptState) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScriptSite_OnScriptError(IActiveScriptSite* This, IActiveScriptError* pscripterror) {
    EXCEPINFO excepInfo;
    DWORD dwSourceContext;
    ULONG ulLineNumber;
    LONG lCharacterPosition;
    BSTR bstrSourceLine=0;
    pscripterror->lpVtbl->GetExceptionInfo(pscripterror, &excepInfo);
    pscripterror->lpVtbl->GetSourcePosition(pscripterror, &dwSourceContext, &ulLineNumber, &lCharacterPosition);
    HRESULT hr = pscripterror->lpVtbl->GetSourceLineText(pscripterror, &bstrSourceLine);
    if (FAILED(hr)) {
        //fprintf(stderr, "Failed to get source line text: %u %x\n", hr, hr);
        bstrSourceLine = SysAllocString(L"(unable to retrieve source line)");
    }

    fwprintf(stderr, L"Script error: %s\nLine %lu, character %ld: %s\n", 
        excepInfo.bstrDescription, 
        1+ulLineNumber, 
        1+lCharacterPosition, 
        bstrSourceLine);
    SysFreeString(bstrSourceLine);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScriptSite_OnEnterScript(IActiveScriptSite* This) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ScriptSite_OnLeaveScript(IActiveScriptSite* This) {
    return S_OK;
}

IActiveScriptSiteVtbl scriptSiteVtbl = {
    ScriptSite_QueryInterface,
    ScriptSite_AddRef,
    ScriptSite_Release,
    ScriptSite_GetLCID,
    ScriptSite_GetItemInfo,
    ScriptSite_GetDocVersionString,
    ScriptSite_OnScriptTerminate,
    ScriptSite_OnStateChange,
    ScriptSite_OnScriptError,
    ScriptSite_OnEnterScript,
    ScriptSite_OnLeaveScript
};

ScriptSite* CreateScriptSite() {
    ScriptSite* site = (ScriptSite*)malloc(sizeof(ScriptSite));
    if (site) {
        site->lpVtbl = &scriptSiteVtbl;
        site->refCount = 1;
    }
    return site;
}
/*  End of implementation of IActiveScriptSite 
 *====================================================================== */

/* Convert an ASCII string to 16-bit "wide" UNICODE.
 * Entry: asciiBuffer is a pointer to the ASCII string
 *        asciiSize is the size of the ASCII string in bytes
 * Exit:  Returns a pointer to a zero-terminated buffer containing the wide 
 *        character string, or NULL if an error occurs.
 */
WCHAR* ConvertToWideChar(const char* asciiBuffer, size_t asciiSize) {
    // Calculate the required buffer size for the wide character string
    int wideCharSize = MultiByteToWideChar(CP_ACP, 0, asciiBuffer, asciiSize, NULL, 0);
    if (wideCharSize == 0) {
        fprintf(stderr, "Error calculating wide character buffer size\n");
        return NULL;
    }

    // Allocate memory for the wide character buffer
    WCHAR* wideCharBuffer = (WCHAR*)malloc((wideCharSize + 1) * sizeof(WCHAR));
    if (!wideCharBuffer) {
        fprintf(stderr, "Error allocating memory for wide character buffer\n");
        return NULL;
    }

    // Perform the conversion
    int result = MultiByteToWideChar(CP_ACP, 0, asciiBuffer, asciiSize, wideCharBuffer, wideCharSize);
    if (result == 0) {
        fprintf(stderr, "Error converting ASCII to wide character\n");
        free(wideCharBuffer);
        return NULL;
    }

    // Null-terminate the wide character string
    wideCharBuffer[wideCharSize] = L'\0';

    return wideCharBuffer;
}

/* Execute the provided JavaScript source, using the Windows Scripting Engine.
 * Entry: script is a pointer to a null-terminated string containing the JavaScript source code
 * Exit:  Returns 0 if successful, or a non-zero value if an error occurs.
 */ 
int ExecuteJavaScript(const char* script) {
    int retval = 0;
    HRESULT hr;
    IActiveScript* pActiveScript = NULL;
    IActiveScriptParse* pActiveScriptParse = NULL;

    // Initialize COM library
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to initialize COM library\n");
        return 2;
    }

    // Create the JavaScript engine
    hr = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER, &IID_IActiveScript, 
        (void**)&pActiveScript);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to create JavaScript engine: %u %x\n", hr, hr);
        CoUninitialize();
        return 3;
    }

    // Get the IActiveScriptParse interface
    hr = pActiveScript->lpVtbl->QueryInterface(pActiveScript, &IID_IActiveScriptParse, 
        (void**)&pActiveScriptParse);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to get IActiveScriptParse interface\n");
        pActiveScript->lpVtbl->Release(pActiveScript);
        CoUninitialize();
        return 4;
    }

    // Initialize the script engine
    hr = pActiveScriptParse->lpVtbl->InitNew(pActiveScriptParse);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to initialize script engine\n");
        pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
        pActiveScript->lpVtbl->Release(pActiveScript);
        CoUninitialize();
        return 5;
    }

    // Create the script "site"
    ScriptSite* pScriptSite = CreateScriptSite();
    if (!pScriptSite) {
        fprintf(stderr, "Failed to create script site\n");
        pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
        pActiveScript->lpVtbl->Release(pActiveScript);
        CoUninitialize();
        return 6;
    }

    // Set the script site 
    hr = pActiveScript->lpVtbl->SetScriptSite(pActiveScript, (IActiveScriptSite*)pScriptSite);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to set script site: %u %x\n", hr, hr);
        pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
        pActiveScript->lpVtbl->Release(pActiveScript);
        CoUninitialize();
        return 7;
    }

    // Convert input script to a wide character string
    WCHAR* wideScript = ConvertToWideChar(script, strlen(script));
    // printf("Wide source:\n%S\n", wideScript);

    // Add the script code
    hr = pActiveScriptParse->lpVtbl->ParseScriptText(pActiveScriptParse, wideScript, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    if (FAILED(hr)) {
        //fprintf(stderr, "Failed to add script code: %u %x\n", hr, hr);
        free(wideScript);
        pScriptSite->lpVtbl->Release((IActiveScriptSite*)pScriptSite);
        pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
        pActiveScript->lpVtbl->Release(pActiveScript);
        CoUninitialize();
        return 8;
    }

    // Execute the script
    hr = pActiveScript->lpVtbl->SetScriptState(pActiveScript, SCRIPTSTATE_CONNECTED);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to execute script\n");
        retval = 9;
    }

    // Clean up
    free(wideScript); // Free the wide character buffer after use
    pScriptSite->lpVtbl->Release((IActiveScriptSite*)pScriptSite);
    pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
    pActiveScript->lpVtbl->Release(pActiveScript);
    CoUninitialize();

    return retval;
}

/* Process the bytes at the end of the executable (if any)
 * Entry: buffer is a pointer to the start of the appended data
 *        bufsize is the size of the appended data in bytes
 * Exit:  Returns 0 if successful, or a non-zero value if an error occurs.
 */
int ProcessAppendedData(const unsigned char* buffer, size_t bufsize) {
    int retval = 0;
    if(bufsize > 0) {
        retval = ExecuteJavaScript((const char*)buffer);
    } else {
        fprintf(stderr, "No appended data to execute\n");
    }
    return retval;
}

int main(int argc, char * argv[]) 
{
    size_t size;
    unsigned char* buffer = LoadCurrentExe(&size);
    if (!buffer) {
        fprintf(stderr, "Error loading executable\n");
        return 2;
    }
    //printf("Executable size: %ld bytes\n", size);
    size_t offset = FindPayloadOffset(buffer, size);
    if (offset == BAD_EXE) {
        printf("Error: Invalid executable format\n");
        return 3;
    }
    //printf("Payload offset: %ld\n", offset);

    unsigned char* appended_data = buffer + offset;
    size_t appended_size = size - offset;
    int retval = ProcessAppendedData(appended_data, appended_size);
    return retval;
}
