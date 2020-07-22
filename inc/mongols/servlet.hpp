#ifndef E266305B_950A_4056_825C_2E7404F3D7A8
#define E266305B_950A_4056_825C_2E7404F3D7A8

#include "request.hpp"
#include "response.hpp"

namespace mongols {

class servlet {
public:
    servlet() = default;
    virtual ~servlet() = default;

    virtual void handler(request& req, response& res) = 0;
    typedef servlet* create_t();
    typedef void destroy_t(servlet*);
};
}

#endif /* E266305B_950A_4056_825C_2E7404F3D7A8 */
