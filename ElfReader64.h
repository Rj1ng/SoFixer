//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2017/6/3.
//                   Copyright (c) 2017. All rights reserved.
//===----------------------------------------------------------------------===//
//  Parse and read elf file.
//===----------------------------------------------------------------------===//

#ifndef SOFIXER_ELFREADER64_H
#define SOFIXER_ELFREADER64_H

#import "exelf.h"

#include <cstdint>
#include <cstddef>
#include <memory.h>

class ElfRebuilder64;

class ElfReader64 {
public:
    ElfReader64();
    ~ElfReader64();

    bool Load();
    void setSource(const char* source, int fd);

    size_t phdr_count() { return phdr_num_; }
    uint8_t * load_start() { return load_start_; }
    Elf64_Addr load_size() { return load_size_; }
    uint8_t * load_bias() { return load_bias_; }
    const Elf64_Phdr* loaded_phdr() { return loaded_phdr_; }

    const Elf64_Ehdr* record_ehdr() { return &header_; }
private:
    bool ReadElfHeader();
    bool VerifyElfHeader();
    bool ReadProgramHeader();
    bool ReserveAddressSpace();
    bool LoadSegments();
    bool FindPhdr();
    bool CheckPhdr(uint8_t *);
    bool LoadFileData(void* addr, size_t len, int offset);

    bool PatchPhdr();

    const char* name_;
    const char* source_;

    int fd_;

    Elf64_Ehdr header_;
    size_t phdr_num_;

    void* phdr_mmap_;
    Elf64_Phdr* phdr_table_;
    Elf64_Addr phdr_size_;

    // First page of reserved address space.
    uint8_t * load_start_;
    // Size in bytes of reserved address space.
    Elf64_Addr load_size_;
    size_t file_size;
    // Load bias.
    uint8_t * load_bias_;

    // Loaded phdr.
    const Elf64_Phdr* loaded_phdr_;

    // feature
public:
    void setDumpSoFile(bool b) { dump_so_file_ = b; }
    void setDumpSoBaseAddr(Elf64_Addr base) { dump_so_base_ = base; }

private:
    bool dump_so_file_ = false;
    Elf64_Addr dump_so_base_ = 0;

    friend class ElfRebuilder64;
};



size_t
phdr_table_get_load_size64(const Elf64_Phdr* phdr_table,
                         size_t phdr_count,
                         Elf64_Addr* min_vaddr = NULL,
                         Elf64_Addr* max_vaddr = NULL);

int
phdr_table_protect_segments64(const Elf64_Phdr* phdr_table,
                            int               phdr_count,
                            uint8_t * load_bias);

int
phdr_table_unprotect_segments64(const Elf64_Phdr* phdr_table,
                              int               phdr_count,
                              uint8_t * load_bias);

int
phdr_table_protect_gnu_relro64(const Elf64_Phdr* phdr_table,
                             int               phdr_count,
                             uint8_t *load_bias);


int phdr_table_get_arm_exidx64(const Elf64_Phdr* phdr_table,
                         int               phdr_count,
                         uint8_t * load_bias,
                         Elf64_Addr**      arm_exidx,
                         unsigned*         arm_exidix_count);

void
phdr_table_get_dynamic_section64(const Elf64_Phdr* phdr_table,
                               int               phdr_count,
                               uint8_t * load_bias,
                               Elf64_Dyn**       dynamic,
                               size_t*           dynamic_count,
                               Elf64_Word*       dynamic_flags);


#endif //SOFIXER_ELFREADER64_H
