#include <mongols/lib/lua/kaguya.hpp>

std::string hello()
{
    return "hello cpp module";
}

#ifdef __cplusplus
extern "C"
{
#endif

    int luaopen_hello(lua_State *L)
    {
        kaguya::State state(L);
        kaguya::LuaTable module = state.newTable();
        module["hello"] = kaguya::function(&hello);
        return module.push();
    }

#ifdef __cplusplus
}
#endif
