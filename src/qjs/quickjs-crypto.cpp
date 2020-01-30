#include "quickjs-crypto.hpp"
#include "lib/crypto/aes.hpp"
#include "util.hpp"
#include <iostream>

static JSClassID js_crypto_class_id;

static void js_crypto_finalizer(JSRuntime* rt, JSValue val)
{
    mongols::aes* aes = (mongols::aes*)JS_GetOpaque(val, js_crypto_class_id);
    delete aes;
}

static JSValue js_crypto_ctor(JSContext* ctx,
    JSValueConst new_target,
    int argc, JSValueConst* argv)
{
    if (argc < 1) {
        return JS_EXCEPTION;
    }
    const char* key = JS_ToCString(ctx, argv[0]);
    mongols::aes* aes = new mongols::aes(key);
    JSValue obj = JS_NewObjectClass(ctx, js_crypto_class_id);
    JS_SetOpaque(obj, aes);
    JS_FreeCString(ctx, key);
    return obj;
}

static JSValue js_crypto_encode(JSContext* ctx, JSValueConst this_val,
    int argc, JSValueConst* argv)
{
    if (argc < 1) {
        return JS_EXCEPTION;
    }
    const char* str = JS_ToCString(ctx, argv[0]);
    mongols::aes* aes = (mongols::aes*)JS_GetOpaque2(ctx, this_val, js_crypto_class_id);
    std::string cipher = aes->encode(str);
    JSValue ret = JS_NewString(ctx, mongols::base64_encode(cipher).c_str());
    JS_FreeCString(ctx, str);
    return ret;
}

static JSValue js_crypto_decode(JSContext* ctx, JSValueConst this_val,
    int argc, JSValueConst* argv)
{
    if (argc < 1) {
        return JS_EXCEPTION;
    }
    const char* str = JS_ToCString(ctx, argv[0]);
    mongols::aes* aes = (mongols::aes*)JS_GetOpaque2(ctx, this_val, js_crypto_class_id);
    std::string plain = aes->decode(mongols::base64_decode(str));
    JSValue ret = JS_NewString(ctx, plain.c_str());
    JS_FreeCString(ctx, str);
    return ret;
}

static JSClassDef js_crypto_class = {
    "crypto",
    .finalizer = js_crypto_finalizer,
};

static const JSCFunctionListEntry js_crypto_funcs[] = {
    JS_CFUNC_DEF("encode", 1, js_crypto_encode),
    JS_CFUNC_DEF("decode", 1, js_crypto_decode),
};

static int js_crypto_init(JSContext* ctx, JSModuleDef* m)
{
    JS_NewClassID(&js_crypto_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_crypto_class_id, &js_crypto_class);
    JSValue crypto_class, crypto_proto;
    crypto_proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, crypto_proto, js_crypto_funcs, countof(js_crypto_funcs));
    JS_SetClassProto(ctx, js_crypto_class_id, crypto_proto);
    crypto_class = JS_NewCFunction2(ctx, js_crypto_ctor, "crypto", 1, JS_CFUNC_constructor, 0);
    JS_SetModuleExport(ctx, m, "crypto", crypto_class);
    return 0;
}

JSModuleDef* js_init_module_crypto(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, js_crypto_init);
    if (!m)
        return NULL;
    JS_AddModuleExport(ctx, m, "crypto");
    return m;
}