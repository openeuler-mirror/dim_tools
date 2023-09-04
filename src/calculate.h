/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: baseline function header
 */
#ifndef DIM_CALCULATE_H
#define DIM_CALCULATE_H

#include "baseline.h"

#define ROUND_UP(n, d) (((n) + (d) - 1) / (d) * (d))

enum DimElfClass {
    DIM_ELF_EXEC_64,
    DIM_ELF_MOD_64,
    DIM_ELF_DYN_64,
    DIM_ELF_UNSUPPORT,
};

int DimSetHashAlgo(const char *algo);
void DimSetCalculateVerbose(int v);
const char *DimGetHashAlgo(void);
struct DimBaselineData *DimCalculateElfBaseline(const char *path);
struct DimBaselineData *DimCalculateKernelBaseline(const char *path, const char *name);

#endif
