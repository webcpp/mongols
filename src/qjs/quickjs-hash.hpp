#ifndef E655A67A_5FA2_4E54_8DB9_F402AD8CD4E3
#define E655A67A_5FA2_4E54_8DB9_F402AD8CD4E3

extern "C" {
#include "cutils.h"
#include "quickjs-libc.h"
}

JSModuleDef* js_init_module_hash(JSContext* ctx, const char* module_name);

#endif /* E655A67A_5FA2_4E54_8DB9_F402AD8CD4E3 */
