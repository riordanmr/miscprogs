/* This program reads the current executable file and finds the length of
   the original EXE section. (Normally, this would be the entire file.)
   It uses the Windows API to get the executable path and reads the file into memory. 
   Then, it parses the PE headers to find the offset of the end of the original file. 

   The purpose of this program is to investigate the possibility of implementing
   an interpreter that can have a source program simply appended to its
   Windows EXE file, and then be run as a standalone program.

   Mark Riordan   2025-03-16
   Credits to https://stackoverflow.com/questions/34684660/how-to-determine-the-size-of-an-pe-executable-file-from-headers-and-or-footers
 */ 

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#define BAD_EXE 0xfffffffe
size_t find_payload_offset(const unsigned char * buffer, size_t bufsize)
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
            //   throw std::runtime_error{"find_payload_offset: Unstripped UPX Compressed Executables are NOT SUPPORTED"};
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

unsigned char* load_current_exe(size_t* size) {
    // Get the path of the current executable
    char exe_path[MAX_PATH];
    if (GetModuleFileName(NULL, exe_path, MAX_PATH) == 0) {
        fprintf(stderr, "Error getting executable path\n");
        return NULL;
    }

    // Open the executable file
    FILE* file = fopen(exe_path, "rb");
    if (!file) {
        fprintf(stderr, "Error opening executable file\n");
        return NULL;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer to hold the file contents
    unsigned char* buffer = (unsigned char*)malloc(*size);
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

    return buffer;
}

int main(int argc, char * argv[]) 
{
    size_t size;
    unsigned char* buffer = load_current_exe(&size);
    if (!buffer) {
        fprintf(stderr, "Error loading executable\n");
        return 2;
    }
    printf("Executable size: %ld bytes\n", size);
    long offset = find_payload_offset(buffer, size);
    if (offset == BAD_EXE) {
        printf("Error: Invalid executable format\n");
        return 3;
    }
    printf("Payload offset: %ld\n", offset);
    return 0;
}