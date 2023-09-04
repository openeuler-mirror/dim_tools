/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: baseline function
 */
#include "baseline.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libkmod.h>
#include <sys/utsname.h>
#include "utils.h"

static const char *DimGetModuleAttr(struct kmod_list *list, const char *str)
{
    struct kmod_list *l = NULL;
    const char *key = NULL;

    kmod_list_foreach(l, list) {
        key = kmod_module_info_get_key(l);
        if (key == NULL || strcmp(key, str) != 0) {
            continue;
        }

        return kmod_module_info_get_value(l);
    }
    return NULL;
}

static int DimGetModuleBaselineName(const char *path, char *buf, unsigned int bufLen)
{
    int ret;
    struct kmod_module *mod = NULL;
    struct kmod_list *list = NULL;

    const char *val = NULL;
    char krel[NAME_MAX];

    struct kmod_ctx *ctx = kmod_new(NULL, NULL);
    if (ctx == NULL) {
        (void)fprintf(stderr, "Fail to create kmod context\n");
        return -1;
    }

    if ((ret = kmod_module_new_from_path(ctx, path, &mod)) < 0) {
        (void)fprintf(stderr, "Fail to create kmod from %s\n", path);
        goto out;
    }

    if ((ret = kmod_module_get_info(mod, &list)) < 0) {
        (void)fprintf(stderr, "Fail to get module info from %s\n", path);
        goto out;
    }

    if ((val = DimGetModuleAttr(list, "vermagic")) == NULL) {
        ret = -1;
        (void)fprintf(stderr, "Fail to get module vermagic of %s\n", path);
        goto out;
    }

    if ((ret = DimCopyFirstField(val, krel, sizeof(krel))) < 0) {
        (void)fprintf(stderr, "Fail to handle kernel release of %s\n", path);
        goto out;
    }

    if ((val = DimGetModuleAttr(list, "name")) == NULL) {
        ret = -1;
        (void)fprintf(stderr, "Fail to get module name of %s\n", path);
        goto out;
    }

    if ((ret = DimPathCat(krel, val, buf, bufLen)) < 0) {
        (void)fprintf(stderr, "Fail to handle baseline name of %s\n", path);
        goto out;
    }

    ret = 0;
out:
    kmod_module_info_free_list(list);
    (void)kmod_module_unref(mod);
    (void)kmod_unref(ctx);
    return ret;
}

struct DimBaselineData *DimCreateBaselineData(const char *filename,
                                              enum DimBaselineType type)
{
    struct DimBaselineData *data = NULL;
    int ret;

    if (filename == NULL || type >= DIM_BASELINE_LAST) {
        return NULL;
    }

    data = (struct DimBaselineData*)malloc(sizeof(struct DimBaselineData));
    if (data == NULL) {
        (void)fprintf(stderr, "Fail to alloc memory for baseline data\n");
        return NULL;
    }

    ret = type == DIM_BASELINE_MOD ?
        DimGetModuleBaselineName(filename, data->filename, sizeof(data->filename)) :
        DimPathCat(filename, NULL, data->filename, sizeof(data->filename));
    if (ret < 0) {
        (void)fprintf(stderr, "Fail to handle filename in baseline data of %s\n", filename);
        free(data);
        return NULL;
    }

    data->type = type;
    data->dataLen = 0;
    return data;
}

void DimDestroyBaselineData(struct DimBaselineData *data)
{
    free(data);
}
