#ifndef SIMPLE_RESP_SIMPLE_RESP_H
#define SIMPLE_RESP_SIMPLE_RESP_H

#include <iostream>
#include <string>
#include <vector>

namespace simple_resp {

    enum RESP_TYPE {
        SIMPLE_STRINGS = '+',
        ERRORS = '-',
        INTEGERS = ':',
        BULK_STRINGS = '$',
        ARRAYS = '*'
    };

    enum STATUS {
        OK = 0,
        EMPTY_INPUT = 1,
        INVAILID_RESP_TYPE = 2,
        INVAILID_RESP_FORMAT = 3,
        UNKNOWN_INTERNAL_ERROR = 3,
    };

    enum PARSE_STATE {
        INIT = 0,
        PARSE_ELEMENTS = 1,
        PARSE_BLUK_STRINGS = 2
    };

    struct encode_result {
        STATUS status;
        std::string response;
    };

    struct decode_result {
        STATUS status;
        std::vector<std::string> response;
    };

    class decoder {
    public:
        decoder() = default;
        decode_result decode(const std::string &input);

    private:
        decode_result parse_arrays(const std::string &input);

    public:
        // decoder is non-copyable.
        decoder(const decoder &) = delete;
        decoder operator=(const decoder &) = delete;
    };

    class encoder {
    public:
        encoder() = default;
        encode_result encode(const RESP_TYPE &type, const std::vector<std::string> &args);

        // Encoder is non-copyable.
        encoder(const encoder &) = delete;
        encoder operator=(const encoder &) = delete;
    };

} // namespace simple_resp

#endif //SIMPLE_RESP_SIMPLE_RESP_H
