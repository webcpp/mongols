#ifndef D48BC883_1C33_4432_A96C_347C25F94456
#define D48BC883_1C33_4432_A96C_347C25F94456

extern "C" {
#include "cutils.h"
#include "quickjs-libc.h"
}

JSModuleDef* js_init_module_crypto(JSContext* ctx, const char* module_name);

#endif /* D48BC883_1C33_4432_A96C_347C25F94456 */
