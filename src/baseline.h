/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: calculate function header
 */
#ifndef DIM_BASELINE_H
#define DIM_BASELINE_H

#include <linux/limits.h>

#define DIM_BASELINE_MAX_HASH_LEN 64

enum DimBaselineType {
    DIM_BASELINE_USER,
    DIM_BASELINE_MOD,
    DIM_BASELINE_KERNEL,
    DIM_BASELINE_LAST,
};

struct DimBaselineData {
    char filename[PATH_MAX];
    enum DimBaselineType type;
    unsigned char mainData[DIM_BASELINE_MAX_HASH_LEN];
    unsigned int dataLen;
};
struct DimBaselineData *DimCreateBaselineData(const char *filename,
                                              enum DimBaselineType type);
void DimDestroyBaselineData(struct DimBaselineData *data);
int DimSetBaselineDataMainHash(struct DimBaselineData *data,
                               unsigned char *hash,
                               unsigned int hashLen);

#endif
