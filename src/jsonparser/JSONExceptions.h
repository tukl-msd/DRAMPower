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
#ifndef JSON_PARSER_EXCEPTIONS
#define JSON_PARSER_EXCEPTIONS

#include <exception>
#include <string>

class JsonException : public std::exception {
public:
  explicit JsonException(const std::string& _msg) : msg(_msg){}
  virtual char const* what() const throw(){
      return msg.c_str();
  }
protected:
  std::string msg;
};

class JsonParseException : public std::exception {
public:
  explicit JsonParseException(const std::string& _msg) : msg(_msg){}
  virtual char const* what() const throw(){
      return msg.c_str();
  }
protected:
  std::string msg;
};

class JsonReadException : public std::exception {
public:
  explicit JsonReadException(const std::string& _msg) : msg(_msg){}
  virtual char const* what() const throw(){
      return msg.c_str();
  }
protected:
  std::string msg;
};

class UnknownException : public std::exception {
public:
  explicit UnknownException(const std::string& _msg) : msg(_msg){}
  virtual char const* what() const throw(){
      return msg.c_str();
  }
protected:
  std::string msg;
};

class SplitException : public std::exception {
public:
  explicit SplitException(const std::string& _msg) : msg(_msg){}
  virtual char const* what() const throw(){
      return msg.c_str();
  }
protected:
  std::string msg;
};

class FileReadException: public std::exception {
public:
  explicit FileReadException(const std::string& _msg) : msg(_msg){}
  virtual char const* what() const throw(){
      return msg.c_str();
  }
protected:
  std::string msg;
};

class ValidationException: public std::exception {
public:
  explicit ValidationException(const std::string& _msg) : msg(_msg){}
  virtual char const* what() const throw(){
      return msg.c_str();
  }
protected:
  std::string msg;
};
#endif