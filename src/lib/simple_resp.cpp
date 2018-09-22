#include "simple_resp.h"

namespace simple_resp {

    using vector_num_type = std::vector<std::string>::size_type;
    using string_num_type = std::string::size_type;

    decode_result decoder::decode(const std::string &input) {
        decode_result result;
        result.status = OK;

        if (input.length() < 0) {
            result.status = EMPTY_INPUT;
            return result;
        }

        switch (input[0]) {
            case SIMPLE_STRINGS:
                break;
            case ERRORS:
                break;
            case INTEGERS:
                break;
            case BULK_STRINGS:
                break;
            case ARRAYS:
                result = parse_arrays(input);
                break;
            default:
                result.status = INVAILID_RESP_TYPE;
        }
        return result;
    }

    decode_result decoder::parse_arrays(const std::string &input) {
        PARSE_STATE state = INIT;
        std::string token;
        decode_result result;

        string_num_type bulk_string_length = 0;
        vector_num_type args_num = 0;

        result.status = OK;

        if (*input.begin() != ARRAYS) {
            result.status = INVAILID_RESP_TYPE;
            result.response.clear();
            return result;
        }

        for (auto it = input.begin() + 1, token_start = input.begin() + 1; it != input.end(); it++) {
            if (args_num > 0 && result.response.size() == args_num) {
                return result;
            }
            if (*it == '\r' && *(it + 1) == '\n') {
                token = std::move(std::string(token_start, it));
                switch (state) {
                    case INIT:
                        args_num = static_cast<vector_num_type> (std::stoi(token));
                        state = PARSE_ELEMENTS;
                        break;
                    case PARSE_ELEMENTS:
                        switch (token[0]) {
                            case INTEGERS:
                                result.response.emplace_back(token);
                                break;
                            case BULK_STRINGS:
                                bulk_string_length = static_cast<string_num_type> (std::stoi(token.substr(1, token.length() - 1)));
                                state = PARSE_BLUK_STRINGS;
                                break;
                            default:
                                result.response.clear();
                                result.status = INVAILID_RESP_FORMAT;
                                return result;
                        }
                        break;
                    case PARSE_BLUK_STRINGS:
                        if (bulk_string_length <= 0) {
                            result.response.clear();
                            result.status = INVAILID_RESP_FORMAT;
                            return result;
                        }
                        if (token.length() != bulk_string_length) {
                            result.response.clear();
                            result.status = INVAILID_RESP_FORMAT;
                            return result;
                        }
                        result.response.emplace_back(token);
                        state = PARSE_ELEMENTS;
                        break;
                }
                it = it + 1;
                token_start = it + 1;
                continue;
            }
        }

        if (args_num > 0 && result.response.size() == args_num) {
            return result;
        } else {
            result.response.clear();
            result.status = UNKNOWN_INTERNAL_ERROR;
            return result;
        }
    }

    encode_result encoder::encode(const RESP_TYPE &type, const std::vector<std::string> &args) {
        encode_result result;

        result.status = OK;
        std::string tmp;

        switch (type) {
            case SIMPLE_STRINGS:
                tmp.append("+").append(args[0]).append("\r\n");
                result.response = std::move(tmp);
                break;
            case ERRORS:
                tmp.append("-").append(args[0]).append("\r\n");
                result.response = std::move(tmp);
                break;
            case INTEGERS:
                tmp.append(":").append(args[0]).append("\r\n");
                result.response = std::move(tmp);
                break;
            case BULK_STRINGS:
                tmp.append("$").append(std::to_string(args[0].size())).append("\r\n").append(args[0]).append("\r\n");
                result.response = std::move(tmp);
                break;
            case ARRAYS:
                tmp.append("*").append(std::to_string(args.size())).append("\r\n");
                for (auto &it : args) {
                    tmp.append("$").append(std::to_string(it.size())).append("\r\n").append(it).append("\r\n");
                }
                result.response = std::move(tmp);
                break;
        }
        return result;
    }
} // namespace simple_resp
