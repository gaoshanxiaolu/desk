/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#include "module_api.h"

extern int module_init(void);
extern void module_exit(void);
MODULE_STRUCT(__this_module) = {
    .name = MODULE_NAME,
    .init = module_init,
    .exit = module_exit
};

