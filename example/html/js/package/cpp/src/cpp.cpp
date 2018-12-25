#include <string>
#include <mongols/lib/dukglue/dukglue.h>
#include <mongols/lib/dukglue/duktape.h>
#include <mongols/js_server.hpp>

#include "dukglue/duktape.h"

class demo : public mongols::js_object {
public:
    demo() = default;
    virtual~demo() = default;

    std::string echo(const std::string& text) {
        return text;
    }

    static bool loaded;
};

bool demo::loaded = false;

#ifdef __cplusplus
extern "C" {
#endif

    duk_ret_t cpp(duk_context *ctx) {
        if (!demo::loaded) {
            dukglue_register_constructor<demo>(ctx, "demo");
            dukglue_register_method(ctx, &demo::echo, "echo");
            demo::loaded = true;
        }
        if (demo::loaded) {
            duk_push_true(ctx);
        } else {
            duk_push_false(ctx);
        }
        return 1; /* one return value */
    }

#ifdef __cplusplus
}
#endif

