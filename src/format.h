/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: format function header
 */
#ifndef DIM_FORMAT_H
#define DIM_FORMAT_H

#include "baseline.h"

#define DIM_BUF_LEN 256
#define DIM_PAGE_SIZE 4096

/* The max length of static baseline is smaller than 8192 */
#define TEXT_BUFFER_LEN_DEFAULT 8192
/* The max size of static baseline file */
#define TEXT_BUFFER_LEN_MAX (10 * 1024 * 1024)

int DimTextInit(void);
void DimTextDestroy(void);
int DimAddBaselineData(struct DimBaselineData *data);
int DimTextOutputBaseline(const char *path);

#endif
