#include <mongols/lib/dukglue/duktape.h>

#ifdef __cplusplus
extern "C" {
#endif

    duk_ret_t adder(duk_context *ctx) {
        int i;
        int n = duk_get_top(ctx); /* #args */
        double res = 0.0;

        for (i = 0; i < n; i++) {
            res += duk_to_number(ctx, i);
        }

        duk_push_number(ctx, res);
        return 1; /* one return value */
    }


#ifdef __cplusplus
}
#endif

