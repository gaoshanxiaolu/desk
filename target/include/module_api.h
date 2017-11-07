/*

* Copyright (c) 2016 Qualcomm Technologies, Inc.

* All Rights Reserved.

* Confidential and Proprietary - Qualcomm Technologies, Inc.

*/

#ifndef __MODULE_API_H__
#define __MODULE_API_H__

#define MINFO_SECTION_NAME ".data.mod.info"

#define MODULE_INIT(initfn) \
        int module_init(void) __attribute__((alias(#initfn)));

#define MODULE_EXIT(exitfn) \
        void module_exit(void) __attribute__((alias(#exitfn)));

#define MODULE_STRUCT(name) \
        struct module name \
	__attribute__((section(MINFO_SECTION_NAME)))

struct module {
    struct module *next;    /*pointer to next module*/
    char *name;     /*module name*/
    int (*init)(void);  /*initial function*/
    void (*exit)(void);     /*exit function*/ 
    unsigned char *lit_start;   /*literal segment start */
    unsigned int lit_size;  /*literal segment size*/
    unsigned char *text_start;  /*text segment start */
    unsigned int text_size;     /*text segment size */
    unsigned char *data_start;  /*data rodata bss segment start */
    unsigned int data_size;     /*data rodata bss segment size */
};

int qcom_load_module(int partid, char *name);

int qcom_remove_module(char *name);

int qcom_show_module();

typedef int (*ADD_PARAMS)(int p0, int p1, int p2, int p3);


#endif
