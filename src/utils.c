/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: utils function
 */
#include "utils.h"
#include <string.h>
#include <stdio.h>

int DimPathCat(const char *dirName, const char *fileName, char *buf, unsigned int bufLen)
{
    int ret = 0;
    size_t dirLen = dirName == NULL ? 0 : strlen(dirName);
    size_t fileLen = fileName == NULL ? 0 : strlen(fileName);
    size_t allLen = dirLen + fileLen + 1;

    if (allLen <= 1 || allLen >= bufLen) {
        return -1;
    }

    ret = sprintf(buf,
                  "%s%s%s",
                  dirLen == 0 ? "/" : dirName,
                  fileLen == 0 || dirLen == 0 || (dirName[dirLen - 1] == '/') ? "" : "/",
                  fileLen == 0 ? "" : fileName);
    return ret < 0 ? -1 : 0;
}

int DimHexToStr(const unsigned char *data, unsigned int dataLen, char *buf, unsigned int bufLen)
{
    int ret = 0;
    unsigned int i;
    int offset = 0;
    size_t remain = bufLen;

    if (data == NULL || buf == NULL) {
        return -1;
    }

    if (dataLen + dataLen + 1 > bufLen) {
        return -1;
    }

    for (i = 0; i < dataLen; i++) {
        ret = sprintf(buf + offset, "%02x", data[i]);
        if (ret < 0) {
            return -1;
        }
        offset += ret;
        remain = bufLen - offset;
    }

    buf[offset] = '\0';
    return 0;
}

int DimCopyFirstField(const char *str, char *buf, unsigned int bufLen)
{
    char *p = NULL;
    size_t allLen;

    if (str == NULL || buf == NULL) {
        return -1;
    }

    p = strchr(str, ' ');
    allLen = p == NULL ? strlen(str) : p - str;
    if (allLen + 1 > bufLen) {
        return -1;
    }

    strncpy(buf, str, allLen);
    buf[allLen] = '\0';
    return 0;
}
