#pragma once

extern "C"
{
#include "cutils.h"
#include "quickjs-libc.h"
}

JSModuleDef *js_init_module_hash(JSContext *ctx, const char *module_name);
