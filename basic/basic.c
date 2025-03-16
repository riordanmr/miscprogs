/* Basic interpreter, based on a 1990 entry to 
   The International Obfuscated C Code Contest by Diomidis Spinellis:
   https://www.ioccc.org/1990/dds/index.html

   The code has been tweaked to compile with modern C compilers.

   The real purpose of this program is to investigate the possibility of implementing 
   an interpreter that can have a source program simply appended to its 
   Windows EXE file, and then be run as a standalone program.
   We may use this technique for the Windows version of the Icon language processor.
   Mark Riordan   2025-03-16
 */
#include <stdio.h>
#include <string.h>

#define O(b,f,u,s,c,a)b(){int o=f();switch(*p++){X u:_ o s b();X c:_ o a b();default:p--;_ o;}}
#define t(e,d,_,C)X e:f=fopen(B+d,_);C;fclose(f)
#define U(y,z)while(p=Q(s,y))*p++=z,*p=' '
#define N for(i=0;i<11*R;i++)m[i]&&
#define I "%d %s\n",i,m[i]
#define X ;break;case
#define _ return
#define R 999

typedef char * A;
int * C, E[R], L[R], M[R], P[R], l, i, j;
char B[R], F[2];
A m[12 * R], p, q, x, y, z, s, d;
FILE * f;
A Q(s, o) A s, o;
{
  for (x = s;* x; x++) {
    for (y = x, z = o;* z && * y ==
      *
      z; y++) z++;
    if (z > o && ! * z) _ x;
  }
  _ 0;
}

G() {
    l = atoi(B);
    m[l] && free(m[l]);
    (p = Q(B, " ")) ? strcpy(m[l] = malloc(strlen(p)), p + 1) : (m[l] = 0, 0);
}
O(S, J, '=', ==, '#', !=)
O(J, K, '<', <, '>', >) O(K, V, '$', <=, '!', >=)
    O(V, W, '+', +, '-', -) O(W, Y, '*', *, '/', /) Y()
{
    int o;
    _ *p == '-' ? p++, -Y() : *p >= '0' && *p <= '9' ? strtol(p, &p, 0)
                                                     : *p == '('
                              ? p++,
        o = S(), p++, o : P[*p++];
}

int basic() {
  m[11 * R] = "E";
  while (puts("Ok"), gets(B)) switch ( * B) {
    X 'R': C = E;
    l = 1;
    for (i = 0; i < R; P[i++] = 0);
    while (l) {
      while (!(s = m[l])) l++;
      if (!Q(s, "\"")) {
        U("<>", '#');
        U("<=", '$');
        U(">=", '!');
      }
      d = B;
      while ( * F = * s) {
        * s == '"' && j
          ++;
        if (j & 1 || !Q(" \t", F)) * d++ = * s;
        s++;
      }* d-- = j = 0;
      if (B[1] != '=') switch ( * B) {
        X 'E': l = -1
        X 'R': B[2] != 'M' && (l = * --C) X 'I': B[1] == 'N' ? gets(p = B), P[ * d] = S() : ( * (q = Q(B, "TH")) = 0, p = B + 2, S() && (p = q + 4, l = S() - 1)) X 'P': B[5] == '"' ? * d = 0, puts(B + 6) : (p = B + 5, printf("%d\n", S())) X 'G': p = B + 4, B[2] == 'S' && ( * C++ = l, p++), l = S() - 1 X 'F': * (q = Q(B, "TO")) = 0;
        p = B + 5;
        P[i = B[3]] = S();
        p = q + 2;
        M[i] = S();
        L[i] = l X 'N': ++P[ * d] <= M[ * d] && (l = L[ * d]);
      } else p = B + 2, P[ *
        B] = S();
      l++;
    }
    X 'L': N printf(I) X 'N': N free(m[i]), m[i] = 0 X 'B': _ 0 t('S', 5, "w", N fprintf(f, I)) t('O', 4, "r",
      while (fgets(B, R, f))( * Q(B, "\n") = 0, G())) X 0: default: G();
  }
  _ 0;
}

#define BAD_EXE -1
long find_payload_offset(const unsigned char * buffer, long bufsize)
{
   long offset=0;
#if 0
    if (bufsize < sizeof(IMAGE_DOS_HEADER))
        return BAD_EXE;
  
    auto dosheader = (IMAGE_DOS_HEADER*)buffer.data();
    
    if (dosheader->e_magic != IMAGE_DOS_SIGNATURE)
        throw ERR_BAD_FORMAT;
    
    if (buffer.size() < size_t(dosheader->e_lfanew) + sizeof(IMAGE_NT_HEADERS))
        throw ERR_BAD_FORMAT;
    
    auto ntheader = (IMAGE_NT_HEADERS*)(buffer.data() + dosheader->e_lfanew);
    
    if (ntheader->Signature != IMAGE_NT_SIGNATURE)
        throw ERR_BAD_FORMAT;
    
    if (buffer.size() < size_t(dosheader->e_lfanew) + sizeof(IMAGE_NT_HEADERS)
                    + sizeof(IMAGE_SECTION_HEADER) * ntheader->FileHeader.NumberOfSections)
        throw ERR_BAD_FORMAT;
    
    auto sectiontable = (IMAGE_SECTION_HEADER*)((uint8_t*)ntheader + sizeof(IMAGE_NT_HEADERS));
    size_t offset;
    
    if (ntheader->FileHeader.PointerToSymbolTable)
    {
        if (std::string_view{(char*)sectiontable->Name, 3} == "UPX") // UPX produces exe with invalid PointerToSymbolTable
            throw std::runtime_error{"find_payload_offset: Unstripped UPX Compressed Executables are NOT SUPPORTED"};
        
        auto symboltable = (IMAGE_SYMBOL*)(buffer.data() + ntheader->FileHeader.PointerToSymbolTable);
        auto stringtable = symboltable + ntheader->FileHeader.NumberOfSymbols;
        auto stringtable_len =  *(int32_t*)stringtable;
        
        auto end = (uint8_t*)stringtable + stringtable_len;
        offset = end - buffer.data();
    }
    else // stripped
    {
        auto last_section_header = sectiontable + ntheader->FileHeader.NumberOfSections - 1;
        offset = last_section_header->PointerToRawData + last_section_header->SizeOfRawData;
    }
    
    auto align = ntheader->OptionalHeader.FileAlignment;
    if (offset % align) offset = (offset/align + 1)*align;
    
    if (offset > buffer.size())
        throw ERR_BAD_FORMAT;
    
    if (offset == buffer.size())
        throw ERR_NO_PAYLOAD;
#endif
    return offset;
}

int main(int argc, char * argv[]) {
    return basic();
}
