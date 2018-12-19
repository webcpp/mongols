/**
 * @file ChaiScript String Methods
 *
 * Adds some additional string methods to ChaiScript strings.
 *
 * string::replace(string search, string replace)
 * string::replace(char search, char replace)
 * string::trim()
 * string::split(string token)
 * string::toLowerCase()
 * string::toUpperCase()
 *
 * To allow selecting indexes from split(), ensure VectorString type is added:
 *
 * chai.add(chaiscript::bootstrap::standard_library::vector_type<std::vector<std::string>>("VectorString"));
 */

#ifndef CHAISCRIPT_EXTRAS_STRING_METHODS_HPP_
#define CHAISCRIPT_EXTRAS_STRING_METHODS_HPP_

#include <algorithm>
#include <string>
#include <vector>

#include "../chaiscript.hpp"

namespace chaiscript {
  namespace extras {
    namespace string_methods {
      ModulePtr bootstrap(ModulePtr m = std::make_shared<Module>())
      {
        // string::replace(std::string search, std::string replace)
        m->add(fun([](const std::string& subject, const std::string& search, const std::string& replace) {
          std::string result(subject);
          size_t pos = 0;
          while ((pos = result.find(search, pos)) != std::string::npos) {
            result.replace(pos, search.length(), replace);
            pos += replace.length();
          }
          return result;
        }), "replace");

        // string::replace(char, char)
        m->add(fun([](const std::string& subject, char search, char replace) {
          std::string result(subject);
          std::replace(result.begin(), result.end(), search, replace);
          return result;
        }), "replace");

        // string::trim()
        m->add(fun([](const std::string& subject) {
          std::string result(subject);
          std::string chars = "\t\n\v\f\r ";
          result.erase(0, result.find_first_not_of(chars));
          result.erase(0, result.find_last_not_of(chars));
          return result;
        }), "trim");

        // string::split(string)
        m->add(fun([](const std::string& subject, const std::string& token) {
          std::string str(subject);
          std::vector<std::string> result;
          while (str.size()) {
            size_t index = str.find(token);
            if (index != std::string::npos) {
              result.push_back(str.substr(0, index));
              str = str.substr(index + token.size());
              if (str.size() == 0) {
                result.push_back(str);
              }
            } else {
              result.push_back(str);
              str = "";
            }
          }
          return result;
        }), "split");

        // string::toLowerCase()
        m->add(fun([](const std::string& subject) {
          std::string result(subject);
          std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::tolower(c);
          });
          return result;
        }), "toLowerCase");

        // string::toUpperCase
        m->add(fun([](const std::string& subject) {
          std::string result(subject);
          std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::toupper(c);
          });
          return result;
        }), "toUpperCase");

        return m;
      }
    }
  }
}

#endif /* CHAISCRIPT_EXTRAS_STRING_METHODS_HPP_ */