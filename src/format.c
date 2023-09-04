/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: output format function
 */
#include "format.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "calculate.h"
#include "baseline.h"
#include "utils.h"

static char *g_textBuf = NULL;
static unsigned long g_textLen = 0;
static unsigned long g_textSize = 0;
static unsigned long g_baselineNum = 0;

static const char *g_dimBaselineTypeName[DIM_BASELINE_LAST] = {
    [DIM_BASELINE_USER] = "USER",
    [DIM_BASELINE_MOD] = "KERNEL",
    [DIM_BASELINE_KERNEL] = "KERNEL",
};

static const char* DimGetBaselineTypeStr(enum DimBaselineType type)
{
    if (type >= DIM_BASELINE_LAST) {
        return NULL;
    }

    return g_dimBaselineTypeName[type];
}

int DimTextInit(void)
{
    g_textBuf = malloc(TEXT_BUFFER_LEN_DEFAULT);
    if (g_textBuf == NULL)
        return -1;

    g_textSize = TEXT_BUFFER_LEN_DEFAULT;
    return 0;
}

void DimTextDestroy(void)
{
    free(g_textBuf);
}

int DimAddBaselineData(struct DimBaselineData *data)
{
    char buf[DIM_BUF_LEN];
    const char *typeStr = NULL;
    char result[TEXT_BUFFER_LEN_DEFAULT] = {0};
    int len = 0;

    if (data == NULL) {
        return -1;
    }

    if ((typeStr = DimGetBaselineTypeStr(data->type)) == NULL) {
        return -1;
    }

    if (DimHexToStr(data->mainData, data->dataLen, buf, sizeof(buf)) < 0) {
        return -1;
    }

    len = sprintf(result, "dim %s %s:%s %s\n", typeStr, DimGetHashAlgo(), buf, data->filename);
    if (g_textLen + len + 1 > g_textSize) {
        /* need to expend buffer */
        g_textSize = (g_textSize << 1);
        if (g_textSize > TEXT_BUFFER_LEN_MAX) {
            (void)fprintf(stderr, "the size of baseline data is bigger than %ld\n", TEXT_BUFFER_LEN_MAX);
            return -1;
        }

        g_textBuf = realloc(g_textBuf, g_textSize);
        if (g_textBuf == NULL) {
            (void)fprintf(stderr, "fail to realloc memory\n");
            return -1;
        }
    }

    strcat(g_textBuf, result);
    g_textLen += len;
    g_baselineNum++;
    return 0;
}

static int DimGetRealPath(const char *path, char *buf, size_t bufLen)
{
    char pathReal[PATH_MAX];
    char dirBuf[PATH_MAX];
    char nameBuf[PATH_MAX];
    char *dirPtr = NULL;
    char *namePtr = NULL;

    strcpy(dirBuf, path);
    strcpy(nameBuf, path);

    /* check dir exist */
    dirPtr = dirname(dirBuf);
    if (dirPtr == NULL || (!realpath(dirPtr, pathReal))) {
        return -1;
    }

    /* check length of filename */
    if ((namePtr = basename(nameBuf)) == NULL) {
        return -1;
    }

    if (strlen(namePtr) >= NAME_MAX ||
        strlen(namePtr) + strlen(dirPtr) + 1 + 1 > PATH_MAX) {
        return -1;
    }

    /* get real path */
    sprintf(buf, "%s/%s", pathReal, namePtr);
    return 0;
}

int DimTextOutputBaseline(const char *path)
{
    const char *str = g_textBuf;
    int fd = 0;
    ssize_t len = 0;
    char pathReal[PATH_MAX];

    if (g_baselineNum == 0) {
        (void)fprintf(stderr, "No valid baseline data\n");
        return -1;
    }

    if (path == NULL) {
        (void)printf("%s", str);
        return 0;
    }

    if (DimGetRealPath(path, pathReal, sizeof(pathReal)) < 0) {
        (void)fprintf(stderr,"Fail to get realpath of %s\n", path);
        return -1;
    }

    fd = open(pathReal, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        (void)fprintf(stderr, "Fail to create baseline file %s: %s\n", pathReal, strerror(errno));
        return -1;
    }

    len = write(fd, str, strlen(str));
    if (len < 0 || len != strlen(str)) {
        (void)fprintf(stderr, "Fail to write %s: %s\n", pathReal, strerror(errno));
        (void)close(fd);
        return -1;
    }

    (void)close(fd);
    return 0;
}
