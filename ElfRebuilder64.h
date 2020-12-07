//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2017/6/4.
//                   Copyright (c) 2017. All rights reserved.
//===----------------------------------------------------------------------===//
// Rebuild elf file with ElfReader
//===----------------------------------------------------------------------===//

#ifndef SOFIXER_ELFREBUILDER64_H
#define SOFIXER_ELFREBUILDER64_H

#include <cstdint>
#include "ElfReader64.h"
#include <vector>
#include <string>

typedef void (*linker_dtor_function_t)();
typedef void (*linker_ctor_function_t)(int, char**, char**);

struct link_map_t {
  // ARC MOD BEGIN
  // GDB expects this to be a 64 bit integer on x86-64 NaCl.
  // See third_party/nacl-glibc/elf/link.h.
  Elf64_Addr l_addr;
  // ARC MOD END
  char*  l_name;
  uintptr_t l_ld;
  link_map_t* l_next;
  link_map_t* l_prev;
};

#define SOINFO_NAME_LEN 128
struct soinfo64 {
public:
    const char *name = "soname";
    const Elf64_Phdr* phdr = nullptr;
    size_t phnum = 0;
    Elf64_Addr entry = 0;
    uint8_t * base = 0;
    unsigned size = 0;

    Elf64_Addr min_load;
    Elf64_Addr max_load;

    uint32_t unused1 = 0;  // DO NOT USE, maintained for compatibility.

    Elf64_Dyn* dynamic = nullptr;
    size_t dynamic_count = 0;
    Elf64_Word dynamic_flags = 0;

    uint32_t unused2 = 0; // DO NOT USE, maintained for compatibility
    uint32_t unused3 = 0; // DO NOT USE, maintained for compatibility

    unsigned flags = 0;

    const char* strtab = nullptr;
    Elf64_Sym* symtab = nullptr;

    uint8_t * hash = 0;
    size_t strtabsize = 0;
    size_t nbucket = 0;
    size_t nchain = 0;
    unsigned* bucket = nullptr;
    unsigned* chain = nullptr;

    Elf64_Addr * plt_got = nullptr;

    Elf64_Rela* plt_rela = nullptr;
    size_t plt_rela_count = 0;

    Elf64_Rela* rela = nullptr;
    size_t rela_count = 0;

    void* preinit_array = nullptr;
    size_t preinit_array_count = 0;

    linker_ctor_function_t* init_array = nullptr;
    size_t init_array_count = 0;
    linker_dtor_function_t* fini_array = nullptr;
    size_t fini_array_count = 0;

    linker_ctor_function_t init_func = nullptr;
    linker_dtor_function_t fini_func = nullptr;

    // ARM EABI section used for stack unwinding.
    Elf64_Addr * ARM_exidx = nullptr;
    size_t ARM_exidx_count = 0;

    size_t ref_count;
    link_map_t link_map;
    bool constructors_called;
    // When you read a virtual address from the ELF file, add this
    // value to get the corresponding address in the process' address space.
    uint8_t * load_bias = nullptr;

    bool has_text_relocations = false;
    bool has_DT_SYMBOLIC = false;
};


class ElfRebuilder64 {
public:
    ElfRebuilder64(ElfReader64* Elf64_reader);
    ~ElfRebuilder64() { if(rebuild_data != nullptr) delete []rebuild_data; }
    bool Rebuild();

    void* getRebuildData() { return rebuild_data; }
    size_t getRebuildSize() { return rebuild_size; }
private:
    bool RebuildPhdr();
    bool RebuildShdr();
    bool ReadSoInfo();
    bool RebuildRelocs();
    bool RebuildFin();

    ElfReader64* elf_reader_;
    soinfo64 si;

    int rebuild_size = 0;
    uint8_t * rebuild_data = nullptr;

    Elf64_Word sDYNSYM = 0;
    Elf64_Word sDYNSTR = 0;
    Elf64_Word sHASH = 0;
    Elf64_Word sRELDYN = 0;
    Elf64_Word sRELPLT = 0;
    Elf64_Word sPLT = 0;
    Elf64_Word sTEXTTAB = 0;
    Elf64_Word sARMEXIDX = 0;
    Elf64_Word sFINIARRAY = 0;
    Elf64_Word sINITARRAY = 0;
    Elf64_Word sDYNAMIC = 0;
    Elf64_Word sGOT = 0;
    Elf64_Word sDATA = 0;
    Elf64_Word sBSS = 0;
    Elf64_Word sSHSTRTAB = 0;

    std::vector<Elf64_Shdr> shdrs;
    std::string shstrtab;

private:
    bool isPatchInit = false;
public:
    void setPatchInit(bool b) { isPatchInit = b; }
};


#endif //SOFIXER_ELFREBUILDER64_H
