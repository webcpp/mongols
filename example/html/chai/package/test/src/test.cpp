#include <string>
#include <mongols/lib/chaiscript/chaiscript.hpp>

std::string concat(const std::string& a,const std::string& b){
    return a+b;
}

CHAISCRIPT_MODULE_EXPORT chaiscript::ModulePtr create_chaiscript_module_test() {
    chaiscript::ModulePtr m(new chaiscript::Module());
    m->add(chaiscript::fun(&concat), "concat");
    return m;
}
