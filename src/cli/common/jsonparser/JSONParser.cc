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
#include "JSONParser.h"
/**
 * Parses the given JSON string
 */
void JSONParser::parse(const std::string& _json_str){
  try{
    jsonObj = json::parse(_json_str);
  } catch(const json::parse_error& _e){
    JsonParseException p_e("@parse() :" + std::string(_e.what()));
    throw p_e;
  } catch(...){
    UnknownException u_e("Unknown exception @ parseJsonString()");
    throw u_e;
  }
}
/**
 * Picks the element recursively according to the given path string
 */
const json& JSONParser::pickElement(json& _json_obj, std::vector<std::string>& _path) const{
  if((_path.size() > 1)){
    try{
      std::string ele(*_path.begin());
      _path.erase(_path.begin());
      auto l_json_obj = _json_obj[ele];
      if (l_json_obj == nullptr){
        JsonReadException ele_e("@pickElement() :"+ ele + " not found");
        throw ele_e;
      } else {
        return pickElement(_json_obj[ele], _path);
      }
    } catch(const JsonReadException& _e){
      throw _e;
    } catch(const json::exception& _e){
      JsonException j_e("@pickElement() :" + std::string(_e.what()));
      throw j_e;
    } catch(...){
      UnknownException u_e("@pickElement() :Unknown exception");
      throw u_e;
    }
  } else {
    try{
      auto l_json_obj = _json_obj[_path.back()];
      if (l_json_obj == nullptr){
        JsonReadException ele_e("@pickElement() :"+ _path.back() + " not found");
        throw ele_e;
      } 
      return _json_obj[_path.back()];
    } catch(const JsonReadException& _e){
      throw _e;
    } catch(const json::exception& _e){
      JsonException j_e("@pickElement() :" + std::string(_e.what()));
      throw j_e;
    } catch(...){
      UnknownException u_e("@pickElement() :Unknown exception");
      throw u_e;
    }
  }
}
/**
 * Validate the presence of elements.
 * Throws ValidationException for elements is which
 * are not configured in the JSON.
 */
void JSONParser::validateElements(const std::vector<std::string>& _elements) {
  std::for_each(_elements.begin(), _elements.end(), [&](const std::string& _element){
    try{
      getElement<json>(_element);
    } catch(const JsonReadException& _e){
      ValidationException v_e("Element expected : "+ 
                                         _element  + 
                                         " "       +
                            std::string(_e.what()));
      throw v_e;
    }
  });
}
/**
 * Returns a pretty printable JSON string.
 */
const std::string JSONParser::toString(){
    return jsonObj.dump(2);
}
/**
 * Reads the JSON text file to a string
 */
std::string JSONParser::readJsonFromFile(const std::string& _file_path){
  std::ifstream ifs;
  std::string retString;
  ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  std::stringstream ss;
  try{
    ifs.open(_file_path);
    ss << ifs.rdbuf();
    retString = ss.str();
  } catch(const std::ifstream::failure& _e){
     FileReadException f_e("@readJsonFromFile() :" + std::string(_e.what()));
     throw f_e;
  } catch(...){
     UnknownException u_e("@readJsonFromFile() :Unknown exception");
     throw u_e;
  }
  return retString;
}
