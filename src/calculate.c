/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: calculate function
 */
#include "calculate.h"
#include <stdio.h>
#include <unistd.h>
#include <libelf.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <openssl/evp.h>
#include "baseline.h"

static int g_verbose = 0;
static const char *g_dimHashAlgo = NULL;
static const EVP_MD *g_dimHashMd = NULL;
static const size_t CAL_BUF_SIZE = 10 * 1024;
static const char * const KO_EXT = ".ko";

extern int g_pagesize;

int DimSetHashAlgo(const char *algo)
{
    if ((g_dimHashMd = EVP_get_digestbyname(algo)) == NULL) {
        (void)fprintf(stderr, "Fail to set %s hash algorithm\n", algo);
        return -1;
    }

    g_dimHashAlgo = algo;
    return 0;
}

void DimSetCalculateVerbose(int v)
{
    g_verbose = v;
}

const char *DimGetHashAlgo(void)
{
    return g_dimHashAlgo;
}

static int DimHashCalFile(const char *path, int fd, unsigned char *data, unsigned int *length)
{
    ssize_t size;
    int ret = -1;
    void *buf = NULL;
    EVP_MD_CTX *mdCtx = NULL;

    if ((buf = malloc(CAL_BUF_SIZE)) == NULL) {
        (void)fprintf(stderr, "No memory\n");
        return ret;
    }

    if ((mdCtx = EVP_MD_CTX_new()) == NULL) {
        (void)fprintf(stderr, "Fail to alloc hash context\n");
        goto out;
    }

    if (EVP_DigestInit(mdCtx, g_dimHashMd) == 0) {
        (void)fprintf(stderr, "Fail to init hash context\n");
        goto out;
    }

    while ((size = read(fd, buf, CAL_BUF_SIZE)) > 0) {
        if (EVP_DigestUpdate(mdCtx, buf, size) == 0) {
            (void)fprintf(stderr, "Fail to update hash digest\n");
            goto out;
        }
    }

    if (size < 0) {
        (void)fprintf(stderr, "Fail to read %s: %s\n", path, strerror(errno));
        goto out;
    }

    if (EVP_DigestFinal(mdCtx, data, length) == 0) {
        (void)fprintf(stderr, "Fail to calculate file hash digest\n");
        goto out;
    }

    ret = 0;
out:
    free(buf);
    if (mdCtx != NULL) {
        EVP_MD_CTX_free(mdCtx);
    }
    return ret;
}

static int DimHashCalElfMod(const char *path, Elf *elf, int elfFd, unsigned char *data, unsigned int *length)
{
    return DimHashCalFile(path, elfFd, data, length);
}

static int DimHashCalElf64Segment(const char *path, int elfFd, EVP_MD_CTX *mdCtx, const Elf64_Phdr *phdr)
{
    ssize_t len;
    size_t readSize, dealSize, calSize;
    long startAddr = 0;
    long offsetAddr = 0;

    startAddr = g_pagesize == 0 ? phdr->p_offset : ROUND_DOWN(phdr->p_offset, g_pagesize);
    offsetAddr = phdr->p_offset - startAddr;
    if (lseek(elfFd, startAddr, SEEK_SET) == -1) {
        (void)fprintf(stderr, "Fail to set seek of %s: %s\n", path, strerror(errno));
        return -1;
    }

    void *buf = malloc(CAL_BUF_SIZE);
    if (buf == NULL) {
        (void)fprintf(stderr, "Fail to alloc memory\n");
        return -1;
    }

    memset(buf, 0, CAL_BUF_SIZE);
    dealSize = 0;
    calSize = g_pagesize == 0 ? phdr->p_filesz : ROUND_UP(phdr->p_filesz + offsetAddr, g_pagesize);
    while (dealSize < calSize) {
        readSize = calSize - dealSize < CAL_BUF_SIZE ?
            calSize - dealSize :
            CAL_BUF_SIZE;

        len = read(elfFd, buf, readSize);
        if (len < 0) {
            (void)fprintf(stderr, "Fail to read code segment of %s\n", path);
            free(buf);
            return -1;
        }

        if (EVP_DigestUpdate(mdCtx, buf, readSize) == 0) {
            (void)fprintf(stderr, "Fail to update hash digest\n");
            free(buf);
            return -1;
        }

        dealSize += readSize;
    }

    free(buf);
    return 0;
}

static int DimHashCalElf64(const char *path, Elf *elf, int elfFd, unsigned char *data, unsigned int *length)
{
    size_t i;
    size_t phNum;
    Elf64_Phdr *phList = NULL;
    EVP_MD_CTX *mdCtx = NULL;

    if (elf_getphdrnum(elf, &phNum) < 0) {
        (void)fprintf(stderr, "Fail to get program headers number of %s: %s\n", path, elf_errmsg(elf_errno()));
        return -1;
    }

    if ((phList = elf64_getphdr(elf)) == NULL) {
        (void)fprintf(stderr, "Fail to get program headers of %s: %s\n", path, elf_errmsg(elf_errno()));
        return -1;
    }

    if ((mdCtx = EVP_MD_CTX_new()) == NULL) {
        (void)fprintf(stderr, "Fail to alloc hash context\n");
        return -1;
    }

    if (EVP_DigestInit(mdCtx, g_dimHashMd) == 0) {
        (void)fprintf(stderr, "Fail to init hash context\n");
        EVP_MD_CTX_free(mdCtx);
        return -1;
    }

    for (i = 0; i < phNum; i++) {
        Elf64_Phdr *phdr = &phList[i];
        if ((phdr != NULL) && (phdr->p_type == PT_LOAD) &&
            (phdr->p_flags & PF_X) && (phdr->p_flags & PF_R) &&
            !(phdr->p_flags & PF_W)) {
            if (DimHashCalElf64Segment(path, elfFd, mdCtx, phdr) < 0) {
                EVP_MD_CTX_free(mdCtx);
                return -1;
            }
        }
    }

    if (EVP_DigestFinal(mdCtx, data, length) == 0) {
        (void)fprintf(stderr, "Fail to get final hash digest\n");
        EVP_MD_CTX_free(mdCtx);
        return -1;
    }

    EVP_MD_CTX_free(mdCtx);
    return 0;
}

static int PathEndsWithKo(const char *path)
{
    size_t len1 = strlen(path);
    size_t len2 = strlen(KO_EXT);
    if (len1 < len2 || strcmp(path + len1 - len2, KO_EXT) != 0) {
        return 0;
    }
    return 1;
}

static int (*g_dimHashFunc[4])(const char *, Elf *, int, unsigned char *, unsigned int *) = {
    [DIM_ELF_EXEC_64] = DimHashCalElf64,
    [DIM_ELF_MOD_64] = DimHashCalElfMod,
    [DIM_ELF_DYN_64] = DimHashCalElf64,
    [DIM_ELF_UNSUPPORT] = NULL,
};

static int DimGetElfClass(const char *path, Elf *elf)
{
    char *ident = NULL;
    Elf64_Ehdr *ehdr64 = NULL;

    if ((ident = elf_getident(elf, NULL)) == NULL) {
        (void)fprintf(stderr, "Fail to get elf ident of %s: %s\n", path, elf_errmsg(elf_errno()));
        return -1;
    }

    if (ident[EI_CLASS] != ELFCLASS64) {
        return DIM_ELF_UNSUPPORT;
    }

    if ((ehdr64 = elf64_getehdr(elf)) == NULL) {
        (void)fprintf(stderr, "Fail to get elf header of %s: %s\n", path, elf_errmsg(elf_errno()));
        return -1;
    }

    switch (ehdr64->e_type) {
        case ET_EXEC:
            return DIM_ELF_EXEC_64;
        case ET_DYN:
            return DIM_ELF_DYN_64;
        case ET_REL:
            return PathEndsWithKo(path) ? DIM_ELF_MOD_64 : DIM_ELF_UNSUPPORT;
        default:
            return DIM_ELF_UNSUPPORT;
    }
}

static enum DimBaselineType ElfClassToBaselineType(enum DimElfClass class)
{
    switch (class) {
        case DIM_ELF_EXEC_64:
        case DIM_ELF_DYN_64:
            return DIM_BASELINE_USER;
        case DIM_ELF_MOD_64:
            return DIM_BASELINE_MOD;
        default:
            return DIM_BASELINE_LAST;
    }
}

struct DimBaselineData *DimCalculateElfBaseline(const char *path)
{
    struct DimBaselineData *data = NULL;
    Elf *elf = NULL;
    enum DimElfClass elfClass;
    int fd = 0;
    char pathReal[PATH_MAX];

    if (path == NULL) {
        return NULL;
    }

    if (!realpath(path, pathReal)) {
        (void)fprintf(stderr, "Fail to get realpath of %s: %s\n", path, strerror(errno));
        return NULL;
    }

    if ((fd = open(pathReal, O_RDONLY)) < 0) {
        (void)fprintf(stderr, "Fail to open %s: %s\n", path, strerror(errno));
        return NULL;
    }

    if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
        (void)fprintf(stderr, "Fail to parse elf file %s: %s\n", path, elf_errmsg(elf_errno()));
        (void)close(fd);
        return NULL;
    }

    if (elf_kind(elf) != ELF_K_ELF) {
        if (g_verbose != 0) {
            (void)fprintf(stderr, "%s is not an elf file\n", path);
        }
        goto out;
    }

    if ((elfClass = DimGetElfClass(pathReal, elf)) >= DIM_ELF_UNSUPPORT) {
        if (g_verbose != 0) {
            (void)fprintf(stderr, "Unsupport elf class of %s\n", path);
        }
        goto out;
    }

    if ((data = DimCreateBaselineData(pathReal, ElfClassToBaselineType(elfClass))) == NULL) {
        goto out;
    }

    if (g_dimHashFunc[elfClass](pathReal, elf, fd, data->mainData, &data->dataLen) != 0) {
        DimDestroyBaselineData(data);
        data = NULL;
        goto out;
    }

out:
    (void)elf_end(elf);
    (void)close(fd);
    return data;
}

struct DimBaselineData *DimCalculateKernelBaseline(const char *path, const char *name)
{
    struct DimBaselineData *data = NULL;
    int fd = 0;
    char pathReal[PATH_MAX];

    if (path == NULL) {
        return NULL;
    }

    if (!realpath(path, pathReal)) {
        (void)fprintf(stderr, "Fail to get realpath of %s: %s\n", path, strerror(errno));
        return NULL;
    }

    if ((fd = open(pathReal, O_RDONLY)) < 0) {
        (void)fprintf(stderr, "Fail to open %s: %s\n", path, strerror(errno));
        return NULL;
    }

    data = DimCreateBaselineData(name, DIM_BASELINE_KERNEL);
    if (data == NULL) {
        (void)fprintf(stderr, "No memory\n");
        (void)close(fd);
        return NULL;
    }

    if (DimHashCalFile(pathReal, fd, data->mainData, &data->dataLen) < 0) {
        DimDestroyBaselineData(data);
        (void)close(fd);
        return NULL;
    }

    (void)close(fd);
    return data;
}
