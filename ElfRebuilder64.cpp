//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2017/6/4.
//                   Copyright (c) 2017. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//
#include <cstdio>
#include "ElfRebuilder64.h"
#include "elf.h"
#include "FDebug.h"

static uint64_t paddup(uint64_t input, uint64_t align) {
    uint64_t pad = ~(align-1);
	return input % align ? (input + align) & pad : input;
}

ElfRebuilder64::ElfRebuilder64(ElfReader64 *elf_reader) {
    elf_reader_ = elf_reader;
}

bool ElfRebuilder64::RebuildPhdr() {
    FLOGD("=======================RebuildPhdr=========================");
    auto phdr = (Elf64_Phdr*)elf_reader_->loaded_phdr();
    for(auto i = 0; i < elf_reader_->phdr_count(); i++) {
        phdr->p_filesz = phdr->p_memsz;     // expend filesize to memsiz
        // p_paddr and p_align is not used in load, just ignore it.
        // fix file offset.
        phdr->p_paddr = phdr->p_vaddr;
        phdr->p_offset = phdr->p_vaddr;     // elf has been loaded.
        phdr++;
    }
    FLOGD("=====================RebuildPhdr End======================");
    return true;
}

bool ElfRebuilder64::RebuildShdr() {
    FLOGD("=======================RebuildShdr=========================");
    // rebuilding shdr, link information
    auto base = si.load_bias;
    shstrtab.push_back('\0');

    // empty shdr
    if(true) {
        Elf64_Shdr shdr = {0};
        shdrs.push_back(shdr);
    }
    

    // gen .dynsym
    if(si.symtab != nullptr) {
        sDYNSYM = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".dynsym");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_DYNSYM;
        shdr.sh_flags = SHF_ALLOC;
        shdr.sh_addr = (uintptr_t)si.symtab - (uintptr_t)base;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = 0;   // calc sh_size later(pad to next shdr)
        shdr.sh_link = 0;   // link to dynstr later
        shdr.sh_info = 1;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = sizeof(Elf64_Sym);

        shdrs.push_back(shdr);
    }

    // gen .dynstr
    if(si.strtab != nullptr) {
        sDYNSTR = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".dynstr");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_STRTAB;
        shdr.sh_flags = SHF_ALLOC;
        shdr.sh_addr = (uintptr_t)si.strtab - (uintptr_t)base;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.strtabsize;
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 1;
        shdr.sh_entsize = 0x0;

        shdrs.push_back(shdr);
    }

    // gen .hash
    if(si.hash != 0) {
        sHASH = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".hash");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_HASH;
        shdr.sh_flags = SHF_ALLOC;
        shdr.sh_addr = si.hash - base;
        shdr.sh_offset = shdr.sh_addr;
        // TODO 32bit, 64bit?
        shdr.sh_size = (si.nbucket + si.nchain) * sizeof(Elf32_Addr) + 2 * sizeof(Elf32_Addr);
        shdr.sh_link = sDYNSYM;
        shdr.sh_info = 0;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = 0x4;

        shdrs.push_back(shdr);
    }

    // gen .rela.dyn
    if(si.rela != nullptr) {
        sRELDYN = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".rela.dyn");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_RELA;
        shdr.sh_flags = SHF_ALLOC;
        shdr.sh_addr = (uintptr_t)si.rela - (uintptr_t)base;
        shdr.sh_addralign = 8;
        if (shdr.sh_addr % 8)
            shdr.sh_addr |= shdr.sh_addralign;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.rela_count * sizeof(Elf64_Rela);
        shdr.sh_link = sDYNSYM;
        shdr.sh_info = 0;
        shdr.sh_entsize = sizeof(Elf64_Sym);

        shdrs.push_back(shdr);
    }

    // gen .rela.plt
    if(si.plt_rela != nullptr) {
        sRELPLT = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".rela.plt");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_RELA;
        shdr.sh_flags = SHF_ALLOC;
        shdr.sh_addr = (uintptr_t)si.plt_rela - (uintptr_t)base;
        shdr.sh_addralign = 8;
        if (shdr.sh_addr % 8)
            shdr.sh_addr |= shdr.sh_addralign;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.plt_rela_count * sizeof(Elf64_Rela);
        shdr.sh_link = sDYNSYM;
        shdr.sh_info = 6;
        shdr.sh_entsize = sizeof(Elf64_Sym);

        shdrs.push_back(shdr);
    }

    // gen.plt with .rel.plt
    if(si.plt_rela != nullptr) {
        sPLT = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".plt");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_PROGBITS;
        shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        shdr.sh_addr = paddup(shdrs[sRELPLT].sh_addr + shdrs[sRELPLT].sh_size, 16);
        shdr.sh_offset = shdr.sh_addr;
        // TODO fix size 32bit 64bit?
        shdr.sh_size = paddup(20/*Pure code*/ + 16 * si.plt_rela_count, 16);
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 0x10;
        shdr.sh_entsize = 16;

        shdrs.push_back(shdr);
    }

    // gen.text&ARM.extab
    if(si.plt_rela != nullptr) {
        sTEXTTAB = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".text&ARM.extab");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_PROGBITS;
        shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        shdr.sh_addr =  shdrs[sPLT].sh_addr + shdrs[sPLT].sh_size;
        // Align 8
        while (shdr.sh_addr & 0x7) {
            shdr.sh_addr ++;
        }

        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = 0;       // calc later
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = 0x0;

        shdrs.push_back(shdr);
    }

    // gen ARM.exidx
    if(si.ARM_exidx != nullptr) {
        sARMEXIDX = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".ARM.exidx");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_ARM_EXIDX;
        shdr.sh_flags = SHF_ALLOC | SHF_LINK_ORDER;
        shdr.sh_addr = (uintptr_t)si.ARM_exidx - (uintptr_t)base;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.ARM_exidx_count * sizeof(Elf64_Addr);
        shdr.sh_link = sTEXTTAB;
        shdr.sh_info = 0;
        shdr.sh_addralign = 4;
        shdr.sh_entsize = 0x8;

        shdrs.push_back(shdr);
    }
    // gen .fini_array
    if(si.fini_array != nullptr) {
        // sRELPLT = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".fini_array");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_FINI_ARRAY;
        shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
        shdr.sh_addr = (uintptr_t)si.fini_array - (uintptr_t)base;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.fini_array_count * sizeof(Elf64_Addr);
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = 0x0;

        shdrs.push_back(shdr);
    }

    // gen .init_array
    if(si.init_array != nullptr) {
        // sRELPLT = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".init_array");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_INIT_ARRAY;
        shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
        shdr.sh_addr = (uintptr_t)si.init_array - (uintptr_t)base;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.init_array_count * sizeof(Elf64_Addr);
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = 0x0;

        shdrs.push_back(shdr);
    }

    // gen .dynamic
    if(si.dynamic != nullptr) {
        sDYNAMIC = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".dynamic");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_DYNAMIC;
        shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
        shdr.sh_addr = (uintptr_t)si.dynamic - (uintptr_t)base;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.dynamic_count * sizeof(Elf64_Dyn);
        shdr.sh_link = sDYNSTR;
        shdr.sh_info = 0;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = 0x10;

        shdrs.push_back(shdr);
    }

    // get .got
    if(si.plt_got != nullptr) {
        // global_offset_table
        sGOT = shdrs.size();
        auto sLast = sGOT - 1;

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".got");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_PROGBITS;
        shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
        shdr.sh_addr = shdrs[sLast].sh_addr + shdrs[sLast].sh_size;
        // Align8??
        while (shdr.sh_addr & 0x7) {
            shdr.sh_addr ++;
        }

        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = (uintptr_t)(si.plt_got + si.plt_rela_count) - shdr.sh_addr - (uintptr_t)base + 3 * sizeof(Elf64_Addr);
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = 0x0;

        shdrs.push_back(shdr);
    }

    // gen .data
    if(true) {
        sDATA = shdrs.size();
        auto sLast = sDATA - 1;

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".data");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_PROGBITS;
        shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
        shdr.sh_addr = paddup(shdrs[sLast].sh_addr + shdrs[sLast].sh_size, 0x1000);
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = si.max_load - shdr.sh_addr;
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 8;
        shdr.sh_entsize = 0x0;

        shdrs.push_back(shdr);
    }

    // gen .bss
//    if(true) {
//        sBSS = shdrs.size();
//
//        Elf64_Shdr shdr;
//        shdr.sh_name = shstrtab.length();
//        shstrtab.append(".bss");
//        shstrtab.push_back('\0');
//
//        shdr.sh_type = SHT_NOBITS;
//        shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
//        shdr.sh_addr = si.max_load;
//        shdr.sh_offset = shdr.sh_addr;
//        shdr.sh_size = 0;   // not used
//        shdr.sh_link = 0;
//        shdr.sh_info = 0;
//        shdr.sh_addralign = 8;
//        shdr.sh_entsize = 0x0;
//
//        shdrs.push_back(shdr);
//    }

    // gen .shstrtab, pad into last data
    if(true) {
        sSHSTRTAB = shdrs.size();

        Elf64_Shdr shdr;
        shdr.sh_name = shstrtab.length();
        shstrtab.append(".shstrtab");
        shstrtab.push_back('\0');

        shdr.sh_type = SHT_STRTAB;
        shdr.sh_flags = 0;
        shdr.sh_addr = si.max_load;
        shdr.sh_offset = shdr.sh_addr;
        shdr.sh_size = shstrtab.length();
        shdr.sh_link = 0;
        shdr.sh_info = 0;
        shdr.sh_addralign = 1;
        shdr.sh_entsize = 0x0;

        shdrs.push_back(shdr);
    }

    // sort shdr and recalc size
    for(auto i = 1; i < shdrs.size(); i++) {
        for(auto j = i + 1; j < shdrs.size(); j++) {
            if(shdrs[i].sh_addr > shdrs[j].sh_addr) {
                // exchange i, j
                auto tmp = shdrs[i];
                shdrs[i] = shdrs[j];
                shdrs[j] = tmp;

                // exchange index
                auto chgIdx = [i, j](Elf64_Word &t) {
                    if(t == i) {
                        t = j;
                    } else if(t == j) {
                        t = i;
                    }
                };
                chgIdx(sDYNSYM);
                chgIdx(sDYNSTR);
                chgIdx(sHASH);
                chgIdx(sRELDYN);
                chgIdx(sRELPLT);
                chgIdx(sPLT);
                chgIdx(sTEXTTAB);
                chgIdx(sARMEXIDX);
                chgIdx(sFINIARRAY);
                chgIdx(sINITARRAY);
                chgIdx(sDYNAMIC);
                chgIdx(sGOT);
                chgIdx(sDATA);
                chgIdx(sBSS);
                chgIdx(sSHSTRTAB);
            }
        }
    }
    // link section data
    if(sDYNSYM != 0) {
        shdrs[sHASH].sh_link = sDYNSYM;
        shdrs[sRELDYN].sh_link = sDYNSYM;
        shdrs[sDYNSYM].sh_link = sDYNSTR;
        shdrs[sRELPLT].sh_link = sDYNSYM;
        shdrs[sDYNAMIC].sh_link = sDYNSTR;
    }

    if(sDYNSYM != 0) {
        auto sNext = sDYNSYM + 1;
        shdrs[sDYNSYM].sh_size = shdrs[sNext].sh_addr - shdrs[sDYNSYM].sh_addr;
    }

    if(sTEXTTAB != 0) {
        auto sNext = sTEXTTAB + 1;
        shdrs[sTEXTTAB].sh_size = shdrs[sNext].sh_addr - shdrs[sTEXTTAB].sh_addr;
    }

    // fix for size
    for(auto i = 2; i < shdrs.size(); i++) {
        if(shdrs[i].sh_offset - shdrs[i-1].sh_offset < shdrs[i-1].sh_size) {
            shdrs[i-1].sh_size = shdrs[i].sh_offset - shdrs[i-1].sh_offset;
        }
    }

    FLOGD("=====================RebuildShdr End======================");
    return true;
}

bool ElfRebuilder64::Rebuild() {
    return RebuildPhdr() && ReadSoInfo() && RebuildShdr() && RebuildRelocs() && RebuildFin();
}

bool ElfRebuilder64::ReadSoInfo() {
    FLOGD("=======================ReadSoInfo=========================");
    si.base = si.load_bias = elf_reader_->load_bias();
    si.phdr = elf_reader_->loaded_phdr();
    si.phnum = elf_reader_->phdr_count();
    auto base = si.load_bias;
    phdr_table_get_load_size64(si.phdr, si.phnum, &si.min_load, &si.max_load);

    /* Extract dynamic section */
    phdr_table_get_dynamic_section64(si.phdr, si.phnum, si.base, &si.dynamic,
                                   &si.dynamic_count, &si.dynamic_flags);
    if(si.dynamic == nullptr) {
        FLOGE("No valid dynamic phdr data");
        return false;
    }

    // phdr_table_get_arm_exidx64(si.phdr, si.phnum, si.base,
    //                          &si.ARM_exidx, (unsigned*)&si.ARM_exidx_count);

    // Extract useful information from dynamic section.
    uint32_t needed_count = 0;
    for (Elf64_Dyn* d = si.dynamic; d->d_tag != DT_NULL; ++d) {
        switch(d->d_tag){
            // case DT_SONAME:
            //     break;
            case DT_HASH:
                si.hash = d->d_un.d_ptr + (uint8_t*)base;
                si.nbucket = ((unsigned *) (base + d->d_un.d_ptr))[0];
                si.nchain = ((unsigned *) (base + d->d_un.d_ptr))[1];
                si.bucket = (unsigned *) (base + d->d_un.d_ptr + 8);
                si.chain = (unsigned *) (base + d->d_un.d_ptr + 8 + si.nbucket * 4);
                break;
            case DT_GNU_HASH:
                break;
            case DT_STRTAB:
                si.strtab = (const char *) (base + d->d_un.d_ptr);
                FLOGD("string table found at %llx", d->d_un.d_ptr);
                break;
            case DT_STRSZ:
                si.strtabsize = d->d_un.d_val;
                break;
            case DT_SYMTAB:
                si.symtab = (Elf64_Sym *) (base + d->d_un.d_ptr);
                FLOGD("symbol table found at %llx", d->d_un.d_ptr);
                break;
            case DT_SYMENT:
                if (d->d_un.d_val != sizeof(Elf64_Sym)) {
                    FLOGE("invalid DT_SYMENT: %d", (unsigned)d->d_un.d_val);
                    return false;
                }
                break;
            case DT_PLTREL:
                if (d->d_un.d_val != DT_REL && d->d_un.d_val != DT_RELA) {
                    FLOGE("unsupported DT_RELA in \"%s\" %d", si.name, (unsigned)d->d_un.d_val);
                    return false;
                }
                break;
            case DT_JMPREL:
                si.plt_rela = (Elf64_Rela*) (base + d->d_un.d_ptr);
                FLOGD("%s plt_rel (DT_JMPREL) found at %llx", si.name, d->d_un.d_ptr);
                break;
            case DT_PLTRELSZ:
                si.plt_rela_count = d->d_un.d_val / sizeof(Elf64_Rela);
                FLOGD("%s plt_rel_count (DT_PLTRELSZ) %zu", si.name, si.plt_rela_count);
                break;
            case DT_REL:
                FLOGE("DT_REL not supported on 64bit");
                return false;
            case DT_RELA:
                si.rela = (Elf64_Rela*) (base + d->d_un.d_ptr);
                FLOGD("%s rel (DT_REL) found at %llx", si.name, d->d_un.d_ptr);
                break;
            case DT_RELSZ:
                FLOGE("DT_RELSZ not supported on 64bit");
                return false;
            case DT_RELASZ:
                si.rela_count = d->d_un.d_val / sizeof(Elf64_Rela);
                FLOGD("%s rel_size (DT_RELSZ) %zu", si.name, si.rela_count);
                break;
            case DT_RELAENT:
                if (d->d_un.d_val != sizeof(Elf64_Rela)) {
                    FLOGE("invalid DT_RELAENT: %d", (unsigned)d->d_un.d_val);
                    return false;
                }
                break;
            case DT_RELACOUNT:
                break;
            case DT_PLTGOT:
                // Used by mips and mips64.
                /* Save this in case we decide to do lazy binding. We don't yet. */
                si.plt_got = (Elf64_Addr *)(base + d->d_un.d_ptr);
                break;
            case DT_DEBUG:
                // Set the DT_DEBUG entry to the address of _r_debug for GDB
                // if the dynamic table is writable
                break;
            case DT_INIT:
                si.init_func = reinterpret_cast<linker_ctor_function_t>(base + d->d_un.d_ptr);
                FLOGD("%s constructors (DT_INIT) found at %llx", si.name, d->d_un.d_ptr);
                break;
            case DT_FINI:
                si.fini_func = reinterpret_cast<linker_dtor_function_t>(base + d->d_un.d_ptr);
                FLOGD("%s destructors (DT_FINI) found at %llx", si.name, d->d_un.d_ptr);
                break;
            case DT_INIT_ARRAY:
                si.init_array = reinterpret_cast<linker_ctor_function_t*>(base + d->d_un.d_ptr);
                FLOGD("%s constructors (DT_INIT_ARRAY) found at %llx", si.name, d->d_un.d_ptr);
                break;
            case DT_INIT_ARRAYSZ:
                si.init_array_count = ((unsigned)d->d_un.d_val) / sizeof(Elf64_Addr);
                FLOGD("%s constructors (DT_INIT_ARRAYSZ) %zu", si.name, si.init_array_count);
                break;
            case DT_FINI_ARRAY:
                si.fini_array = reinterpret_cast<linker_dtor_function_t*>(base + d->d_un.d_ptr);
                FLOGD("%s destructors (DT_FINI_ARRAY) found at %llx", si.name, d->d_un.d_ptr);
                break;
            case DT_FINI_ARRAYSZ:
                si.fini_array_count = ((unsigned)d->d_un.d_val) / sizeof(Elf64_Addr);
                FLOGD("%s destructors (DT_FINI_ARRAYSZ) %zu", si.name, si.fini_array_count);
                break;
            case DT_PREINIT_ARRAY:
                si.preinit_array = reinterpret_cast<void**>(base + d->d_un.d_ptr);
                FLOGD("%s constructors (DT_PREINIT_ARRAY) found at %d", si.name, (unsigned)d->d_un.d_ptr);
                break;
            case DT_PREINIT_ARRAYSZ:
                si.preinit_array_count = ((unsigned)d->d_un.d_val) / sizeof(Elf64_Addr);
                FLOGD("%s constructors (DT_PREINIT_ARRAYSZ) %zu", si.name, si.preinit_array_count);
                break;
            case DT_TEXTREL:
                si.has_text_relocations = true;
                break;
            case DT_SYMBOLIC:
                si.has_DT_SYMBOLIC = true;
                break;
            case DT_NEEDED:
                ++needed_count;
                break;
            case DT_FLAGS:
                if (d->d_un.d_val & DF_TEXTREL) {
                    si.has_text_relocations = true;
                }
                if (d->d_un.d_val & DF_SYMBOLIC) {
                    si.has_DT_SYMBOLIC = true;
                }
                break;
            case DT_SONAME:
                // si.name = (const char *) (base + d->d_un.d_ptr);
                // FLOGD("soname %s", si.name);
                break;
            // Ignored: "Its use has been superseded by the DF_BIND_NOW flag"
            case DT_BIND_NOW:
                break;
            default:
                FLOGD("Unused DT entry: type 0x%08llx arg 0x%08llx", d->d_tag, d->d_un.d_val);
                break;
        }
    }
    FLOGD("=======================ReadSoInfo End=========================");
    return true;
}

// Finally, generate rebuild_data
bool ElfRebuilder64::RebuildFin() {
    FLOGD("=======================try to finish file=========================");
    auto load_size = si.max_load - si.min_load;
    rebuild_size = load_size + shstrtab.length() +
            shdrs.size() * sizeof(Elf64_Shdr);
    rebuild_data = new uint8_t[rebuild_size];
    memcpy(rebuild_data, (void*)si.load_bias, load_size);
    // pad with shstrtab
    memcpy(rebuild_data + load_size, shstrtab.c_str(), shstrtab.length());
    // pad with shdrs
    auto shdr_off = load_size + shstrtab.length();
    memcpy(rebuild_data + (int)shdr_off, (void*)&shdrs[0],
           shdrs.size() * sizeof(Elf64_Shdr));
    auto ehdr = *elf_reader_->record_ehdr();
    ehdr.e_shnum = shdrs.size();
    ehdr.e_shoff = (Elf64_Addr)shdr_off;
    ehdr.e_shstrndx = sSHSTRTAB;
    memcpy(rebuild_data, &ehdr, sizeof(Elf64_Ehdr));

    FLOGD("=======================End=========================");
    return true;
}

bool ElfRebuilder64::RebuildRelocs() {
    FLOGD("=======================RebuildRelocs=========================");
    if(!elf_reader_->dump_so_file_) return true;
    auto relocate = [](uint8_t * base, Elf64_Rela* rel, size_t count, Elf64_Addr dump_base) {
        if(rel == nullptr || count == 0) return false;
        for(auto idx = 0; idx < count; idx++, rel++) {
            auto type = ELF64_R_TYPE(rel->r_info);
            auto sym = ELF64_R_SYM(rel->r_info);
            auto prel = reinterpret_cast<Elf64_Addr *>(base + rel->r_offset);
            if(type == 0) { // R_*_NONE
                continue;
            }
            switch (type) {
                // I don't known other so info, if i want to fix it, I must dump other so file
                case R_386_RELATIVE:
                case R_ARM_RELATIVE:
                    *prel = *prel - dump_base;
                    break;
                default:
                    break;
            }
        }

        return true;
    };
    relocate(si.load_bias, si.plt_rela, si.plt_rela_count, elf_reader_->dump_so_base_);
    relocate(si.load_bias, si.rela, si.rela_count, elf_reader_->dump_so_base_);
    FLOGD("=======================RebuildRelocs End=======================");
    return true;
}




