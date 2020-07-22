#ifndef B90BEBDC_6618_4951_978F_6B6704B531C0
#define B90BEBDC_6618_4951_978F_6B6704B531C0

extern "C" {
#include "cutils.h"
#include "quickjs-libc.h"
}

int eval_buf(JSContext* ctx, const void* buf, int buf_len,
    const char* filename, int eval_flags);
int eval_file(JSContext* ctx, const char* filename, int module);

JSModuleDef* js_init_module_mongols(JSContext* ctx, const char* module_name);

#endif /* B90BEBDC_6618_4951_978F_6B6704B531C0 */
