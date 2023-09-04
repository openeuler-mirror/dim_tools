/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: main for dim_gen_baseline
 */
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <libelf.h>
#include <sys/stat.h>

#include "calculate.h"
#include "baseline.h"
#include "format.h"
#include "utils.h"

#define DIM_RECURSION_MAX 512

static int g_verbose = 0;
static int g_recursive = 0;
static const char *g_kernelName = NULL;
static const char *g_hashAlgo = NULL;
static const char *g_outPath = NULL;
static char *g_relativeRoot = NULL;
static char g_relativeRootReal[PATH_MAX];

/* Pagesize */
int g_pagesize = 4096;
/* Hash algorithm */
static const char * const DEFAULT_HASH_MD = "sha256";
static const char * const SUPPORT_HASH_MD = "sha256, sm3";
const char * const SUPPORT_HASH_MD_LIST[] = {
    "sha256",
    "sm3",
    "",
};

static struct option g_opts[] = {
    {"help", 0, 0, 'h'},
    {"hashalgo", 1, 0, 'a'},
    {"recursive", 0, 0, 'r'},
    {"verbose", 0, 0, 'v'},
    {"output", 1, 0, 'o'},
    {"kernel", 1, 0, 'k'},
    {"root", 1, 0, 'R'},
    {"pagesize", 1, 0, 'p'},
};

static void Usage(void)
{
    (void)printf("Usage: dim_gen_baseline [options] PATH...\n"
           "options:\n");

    (void)printf("  -a, --hashalgo <ALGO>     %s (default is %s)\n", SUPPORT_HASH_MD, DEFAULT_HASH_MD);

    (void)printf(
        "  -R, --root <PATH>         relative root path\n"
        "  -o, --output <PATH>       output file (default is stdout)\n"
        "  -r, --recursive           recurse into directories\n"
        "  -k, --kernel <RELEASE>    calculate kernel file hash, specify kernel release, conflict with -r\n"
        "  -p, --pagesize <SIZE>     page size align for calculating, now support 0, 4K (default is 4K)\n"
        "  -v, --verbose             increase verbosity level\n"
        "  -h, --help                display this help and exit\n"
        "\n");
}

static int match_pagesize(const char *flag)
{
    if (strcmp(flag, "4K") == 0)
        return 4096;
    else if (strcmp(flag, "0") == 0)
        return 0;
    else
        return -1;
}

static void DoCalculate(const char *path)
{
    struct DimBaselineData *data = NULL;

    data = g_kernelName ? DimCalculateKernelBaseline(path, g_kernelName) :
                          DimCalculateElfBaseline(path);
    if (data == NULL) {
        return;
    }

    if (g_relativeRoot != NULL && data->type == DIM_BASELINE_USER) {
        if (DimPathCat(path + strlen(g_relativeRootReal), NULL, data->filename, sizeof(data->filename)) < 0) {
            DimDestroyBaselineData(data);
            return;
        }
    }

    if (g_verbose != 0) {
        (void)printf("Finish calculating baseline of %s\n", path);
    }

    if (DimAddBaselineData(data) < 0) {
        (void)fprintf(stderr, "Fail add baseline data of %s\n", path);
    }

    DimDestroyBaselineData(data);
}

static void DoCalculateRecursive(const char *dirPath, unsigned int recurCount)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char subPath[PATH_MAX];
    unsigned int count = recurCount;

    if (count >= DIM_RECURSION_MAX) {
        (void)fprintf(stderr, "number of subDir recursions oexceeds the maxValue %d\n", DIM_RECURSION_MAX);
        return;
    }

    if ((dir = opendir(dirPath)) == NULL) {
        (void)fprintf(stderr, "Fail to open directory %s: %s\n", dirPath, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_type != DT_REG && entry->d_type != DT_DIR) {
            continue;
        }

        if (DimPathCat(dirPath, entry->d_name, subPath, sizeof(subPath)) < 0) {
            if (g_verbose != 0) {
                (void)fprintf(stderr, "Oversize abstract path of %s\n", entry->d_name);
            }
            continue;
        }

        if (entry->d_type == DT_REG) {
            DoCalculate(subPath);
        } else {
            count++;
            DoCalculateRecursive(subPath, count);
        }
    }

    (void)closedir(dir);
    return;
}

static int DoCheck(int argc)
{
    struct stat st;

    if (g_kernelName != NULL && g_recursive != 0) {
        (void)printf("Option k and r can't be set both\n");
        return -1;
    }

    if (g_kernelName != NULL && strlen(g_kernelName) > NAME_MAX) {
        (void)printf("Kernel release name too long\n");
        return -1;
    }

    if (g_kernelName != NULL && argc - optind != 1) {
        (void)printf("Only one kernel file can be specified\n");
        return -1;
    }

    if (g_relativeRoot != NULL) {
        if (!realpath(g_relativeRoot, g_relativeRootReal)) {
            (void)fprintf(stderr, "Fail to get absolute path of %s: %s\n", g_relativeRoot, strerror(errno));
            return -1;
        }

        if (lstat(g_relativeRootReal, &st) < 0) {
            (void)fprintf(stderr, "Fail to stat relative root %s: %s\n", g_relativeRootReal, strerror(errno));
            return -1;
        }

        if (!S_ISDIR(st.st_mode)) {
            (void)fprintf(stderr, "Relative root %s is not directory\n", g_relativeRootReal);
            return -1;
        }

        if (strlen(g_relativeRootReal) == 1) {
            g_relativeRoot = NULL;
        }
    }

    return 0;
}

static int DoInit(void)
{
    int i, flag;

    if (DimTextInit() != 0) {
        return -1;
    }

    DimSetCalculateVerbose(g_verbose);

    flag = 0;
    if (!g_hashAlgo) {
        g_hashAlgo = DEFAULT_HASH_MD;
        flag = 1;
    } else {
        for (i = 0; strlen(SUPPORT_HASH_MD_LIST[i]) != 0; i++) {
            if (strcmp(g_hashAlgo, SUPPORT_HASH_MD_LIST[i]) == 0) {
                flag = 1;
                break;
            }
        }
    }

    if (flag == 0) {
        (void)fprintf(stderr, "Unsupported hash algorithm %s\n", g_hashAlgo);
        return -1;
    }

    if (DimSetHashAlgo(g_hashAlgo) != 0) {
        return -1;
    }

    if (elf_version(EV_CURRENT) == EV_NONE) {
        (void)fprintf(stderr, "ELF library too old\n");
        return -1;
    }

    return 0;
}

static void DoCmd(const char *path)
{
    struct stat st;
    char rPath[PATH_MAX];
    size_t dPathLen, rPathLen;

    if (lstat(path, &st) < 0) {
        (void)fprintf(stderr, "Fail to stat %s: %s\n", path, strerror(errno));
        return;
    }

    if (!(S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))) {
        if (g_verbose != 0) {
            (void)fprintf(stderr, "%s is not a regular file or directory\n", path);
        }
        return;
    }

    /* realpath function will check the absolute path is shorter than 4096 */
    if (!realpath(path, rPath)) {
        (void)fprintf(stderr, "Fail to get absolute path of %s: %s\n", path, strerror(errno));
        return;
    }

    if (g_relativeRoot != NULL) {
        dPathLen = strlen(rPath);
        rPathLen = strlen(g_relativeRootReal);
        if (strncmp(rPath, g_relativeRootReal, rPathLen) != 0 ||
            (dPathLen > rPathLen && rPath[rPathLen] != '/')) {
            (void)fprintf(stderr, "Path %s is not under the relative root\n", path);
            return;
        }
    }

    if (g_recursive == 0) {
        if (!S_ISREG(st.st_mode)) {
            if (g_verbose != 0) {
                (void)printf("%s is not a regular file\n", rPath);
            }
            return;
        }

        DoCalculate(rPath);
    } else {
        if (S_ISREG(st.st_mode)) {
            DoCalculate(rPath);
        } else if (S_ISDIR(st.st_mode)) {
            DoCalculateRecursive(rPath, 0);
        }
    }
}

static int DoOutput(void)
{
    int ret;

    ret = DimTextOutputBaseline(g_outPath);
    DimTextDestroy();
    return ret;
}

int main(int argc, char *argv[])
{
    int c, i;
    int maxCnt = 4096;
    char *p = NULL;
    while (maxCnt-- > 0) {
        c = getopt_long(argc, argv, "hvrk:R:a:o:p:", g_opts, NULL);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'h':
                Usage();
                return 0;
            case 'v':
                g_verbose++;
                break;
            case 'r':
                g_recursive++;
                break;
            case 'a':
                g_hashAlgo = optarg;
                break;
            case 'o':
                g_outPath = optarg;
                break;
            case 'k':
                g_kernelName = optarg;
                break;
            case 'R':
                g_relativeRoot = optarg;
                break;
            case 'p':
                g_pagesize = match_pagesize(optarg);
                if (g_pagesize < 0) {
                    (void)printf("Invalid pagesize flag\n");
                    return 1;
                }
                break;
            default:
                (void)printf("Invalid flag\n");
                return 1;
        }
    }

    if (argv[optind] == NULL) {
        (void)printf("Please enter path\n");
        return 1;
    }

    if (DoCheck(argc) < 0) {
        return 1;
    }

    if (DoInit() < 0) {
        return 1;
    }

    for (i = optind; i < argc; i++) {
        DoCmd(argv[i]);
    }

    return DoOutput() < 0 ? 1 : 0;
}
