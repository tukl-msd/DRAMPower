/*
 * Copyright (c) 2012-2014, TU Delft
 * Copyright (c) 2012-2014, TU Eindhoven
 * Copyright (c) 2012-2014, TU Kaiserslautern
 * Copyright (c) 2012-2019, Fraunhofer IESE
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Subash Kannoth
 *
 */
#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <iostream>
#include <exception>
#include <sstream>
#include <fstream>
#include <string>
#include "common/libraries/json/json.h"
#include "JSONExceptions.h"

using json = nlohmann::json;

class JSONParser{
public:
  /**
   * Gets the object specified by the path string. The expected type is 
   * templated. A non-castable template parameter will throw a custom
   * JsonReadException exception.
   */
  template<class T>
  T getElement(const std::string& _path){
    auto split = [&](const std::string& _ele_path) -> std::vector<std::string> {
      std::vector<std::string> ret;
      std::string token;
      try{
        std::stringstream ss(_ele_path);
        while (std::getline(ss, token, '/')) {
          ret.push_back(token);
        }
        return ret;
      } catch(const std::exception& _e){
        SplitException s_e("@getElement() :" + std::string(_e.what()));
      } catch(...){
        UnknownException u_e("@getElement() :Unknown exception");
        throw u_e;
      }
      return ret;
    };
    try{
      std::vector<std::string> path_vect = split(_path);
      json l_jsonObj = pickElement(this->jsonObj, path_vect);
      return l_jsonObj.get<T>();
    } catch(const json::exception& _e){
      JsonException j_e("@getElement() : " + _path +" "+ std::string(_e.what()));
      throw j_e;
    } catch(const JsonReadException& _e) {
      throw _e;
    } catch(const JsonException& _e){
      throw _e;
    } catch(const UnknownException& _e){
      throw _e;
    } catch(...){
      UnknownException u_e("@getElement() :Unknown exception");
      throw u_e;
    }
  }
  /**
   * Gets the object specified by the path string. If the element is
   * not present, the specified default value is returned.
   */
  template<class T>
  T getElementValWithDefault(const std::string& _path, T _default_value){
    try{
      return getElement<T>(_path);
    } catch(const JsonReadException& _e) {
      return (_default_value);
    } catch(const json::exception& _e){
      JsonException j_e("@getElementWithDefaultValue() :" + std::string(_e.what()));
      throw j_e;
    } catch(const JsonException& _e){
      throw _e;
    } catch(const UnknownException& _e){
      throw _e;
    } catch(...){
      UnknownException u_e("@getElementWithDefaultValue() :Unknown exception");
      throw u_e;
    }
  }

  void parse(const std::string& _json_str);
  void validateElements(const std::vector<std::string>& _elements);
  static std::string readJsonFromFile(const std::string& _file_path);
  const std::string toString();
  JSONParser(){};
  virtual ~JSONParser(){};
private:

  const json& pickElement(json& _json_obj, std::vector<std::string>& _path) const;
  mutable json jsonObj;
};

#endif