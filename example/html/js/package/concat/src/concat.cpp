#include <string>
#include <mongols/lib/dukglue/duktape.h>


#ifdef __cplusplus
extern "C" {
#endif

    duk_ret_t concat(duk_context *ctx) {
        int i;
        int n = duk_get_top(ctx); /* #args */
        std::string res;

        for (i = 0; i < n; i++) {
            res += duk_to_string(ctx, i);
        }

        duk_push_string(ctx, res.c_str());
        return 1; /* one return value */
    }

#ifdef __cplusplus
}
#endif

