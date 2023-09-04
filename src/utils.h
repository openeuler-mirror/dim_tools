/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: utils function header
 */
#ifndef DIM_UTILS_H
#define DIM_UTILS_H

int DimPathCat(const char *dirName, const char *fileName, char *buf, unsigned int bufLen);
int DimHexToStr(const unsigned char *data, unsigned int dataLen, char *buf, unsigned int bufLen);
int DimCopyFirstField(const char *str, char *buf, unsigned int bufLen);

#endif
