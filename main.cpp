#include <iostream>
#include "ElfReader.h"
#include "ElfRebuilder.h"
#include "ElfReader64.h"
#include "ElfRebuilder64.h"
#include "FDebug.h"
#include <getopt.h>


const char* short_options = "hdb:s:a:o:";
const struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"debug", 0, NULL, 'd'},
        {"baseso", 1, NULL, 'b'},
        {"source", 1, NULL, 's'},
        {"arch", 1, NULL, 'a'},
        {"output", 1, NULL, 'o'},
        {nullptr, 0, nullptr, 0}
};
void useage();

int main(int argc, char* argv[]) {
    int c;

    ElfReader elf_reader;
    ElfReader64 elf_reader64;

    std::string source, output, arch;
    bool isValidArg = true;
    while((c = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (c) {
            case 'd':
                printf("Use debug mode\n");
                break;
            case 's':
                source = optarg;
                break;
            case 'a':
                arch = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'b': {
                auto is16Bit = [](const char* c) {
                    auto len = strlen(c);
                    if(len > 2) {
                        if(c[0] == '0' & c[1] == 'x') return true;
                    }
                    bool is10bit = true;
                    for(auto i = 0; i < len; i++) {
                        if((c[i] > 'a' && c[i] < 'f') ||
                           (c[i] > 'A' && c[i] < 'F')) {
                            is10bit = false;
                        }
                    }
                    return !is10bit;
                };
                auto base = strtoul(optarg, 0, is16Bit(optarg) ? 16: 10);
                auto base64 = strtoull(optarg, 0, is16Bit(optarg) ? 16: 10);
                elf_reader.setDumpSoFile(true);
                elf_reader.setDumpSoBaseAddr(base);
                elf_reader64.setDumpSoFile(true);
                elf_reader64.setDumpSoBaseAddr(base64);
            }
                break;
            default:
                isValidArg = false;
                break;
        }
    }
    if(!isValidArg) {
        useage();
        return -1;
    }

    auto file = fopen(source.c_str(), "rb");
    if(nullptr == file) {
        printf("source so file cannot found!!!\n");
        return -1;
    }
    auto fd = fileno(file);

    printf("start to rebuild elf file arch: %s\n", arch.c_str());
    if (arch == "32") {
        elf_reader.setSource(source.c_str(), fd);
        if(!elf_reader.Load()) {
            printf("source so file is invalid\n");
            return -1;
        }
        ElfRebuilder elf_rebuilder(&elf_reader);
        if(!elf_rebuilder.Rebuild()) {
            printf("error occured in rebuilding elf file\n");
            return -1;
        }
        fclose(file);

        if (!output.empty()) {
            file = fopen(output.c_str(), "wb+");
            if(nullptr == file) {
                printf("output so file cannot write !!!\n");
                return -1;
            }
            fwrite(elf_rebuilder.getRebuildData(), elf_rebuilder.getRebuildSize(), 1, file);
            fclose(file);
        }
    } else if (arch == "64") {
        elf_reader64.setSource(source.c_str(), fd);
        if(!elf_reader64.Load()) {
            printf("source so file is invalid\n");
            return -1;
        }
        ElfRebuilder64 elf_rebuilder64(&elf_reader64);
        if(!elf_rebuilder64.Rebuild()) {
            printf("error occured in rebuilding elf file\n");
            return -1;
        }
        fclose(file);

        if (!output.empty()) {
            file = fopen(output.c_str(), "wb+");
            if(nullptr == file) {
                printf("output so file cannot write !!!\n");
                return -1;
            }
            fwrite(elf_rebuilder64.getRebuildData(), elf_rebuilder64.getRebuildSize(), 1, file);
            fclose(file);
        }
    }

    printf("Done!!!\n");
    return 0;
}

void useage() {
    printf("SoFixer v0.2 author F8LEFT(currwin)\n");
    printf("Useage: SoFixer <option(s)> -s sourcefile -a arch -o generatefile\n");
    printf(" try rebuild shdr with phdr\n");
    printf(" Options are:\n");

    printf("  -d --debug                                 Show debug info\n");
    printf("  -b --baseso memBaseAddr(16bit format)      Source file is dump from memory from address x\n");
    printf("  -s --source sourceFilePath                 Source file path\n");
    printf("  -a --arch Arch(32bit or 64bit)             Source file arch\n");
    printf("  -o --output generateFilePath               Generate file path\n");
    printf("  -h --help                                  Display this information\n");

}
