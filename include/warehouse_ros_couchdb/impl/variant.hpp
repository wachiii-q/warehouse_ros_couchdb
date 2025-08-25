// Copyright 2020 Bjarne von Horn
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// SPDX-License-Identifier: BSD-3-Clause

#ifndef WAREHOUSE_ROS_COUCHDB__IMPL__VARIANT_HPP_
#define WAREHOUSE_ROS_COUCHDB__IMPL__VARIANT_HPP_

#include <json/json.h>

#include <boost/variant.hpp>

#include <warehouse_ros_couchdb/utils.hpp>
#include <warehouse_ros_couchdb/exceptions.hpp>

#include <string>
#include <sstream>
#include <utility>

namespace warehouse_ros_couchdb
{

// JSON visitor for converting variants to JSON values
class JsonVisitor : boost::static_visitor<Json::Value>
{
public:
  JsonVisitor() = default;
  
  Json::Value operator()(int i)
  {
    return Json::Value(i);
  }
  Json::Value operator()(double d)
  {
    return Json::Value(d);
  }
  Json::Value operator()(const std::string& s)
  {
    return Json::Value(s);
  }
  Json::Value operator()(bool b)
  {
    return Json::Value(b);
  }
};

// JSON reader visitor for converting JSON values to variants
class JsonReaderVisitor : boost::static_visitor<void>
{
  Json::Value value_;
  
public:
  explicit JsonReaderVisitor(const Json::Value& value) : value_(value) {}
  
  void operator()(int& i)
  {
    if (value_.isInt()) {
      i = value_.asInt();
    }
  }
  void operator()(double& d)
  {
    if (value_.isDouble()) {
      d = value_.asDouble();
    }
  }
  void operator()(std::string& s)
  {
    if (value_.isString()) {
      s = value_.asString();
    }
  }
  void operator()(bool& b)
  {
    if (value_.isBool()) {
      b = value_.asBool();
    }
  }
};

// Simple column visitor - for CouchDB we don't need to create columns like in SQLite
class EnsureColumnVisitor : boost::static_visitor<void>
{
  std::string host_;
  std::string collection_name_;
  
public:
  explicit EnsureColumnVisitor(const std::string& host, const std::string& collection_name)
  : host_(host), collection_name_(collection_name)
  {
  }
  
  void operator()(int)
  {
    // For CouchDB, we don't need to pre-create columns
    // JSON documents are schema-less
  }
  void operator()(double)
  {
    // For CouchDB, we don't need to pre-create columns
  }
  void operator()(const std::string&)
  {
    // For CouchDB, we don't need to pre-create columns
  }
  void operator()(bool)
  {
    // For CouchDB, we don't need to pre-create columns
  }
  
  void addColumn(const char*)
  {
    // For CouchDB, we don't need to add columns like in SQLite
    // This is a no-op for JSON documents
  }
};

// Stream visitor for converting variants to strings
class StreamVisitor : boost::static_visitor<std::ostream&>
{
  std::ostream& os_;
public:
  explicit StreamVisitor(std::ostream& os) : os_(os) {}
  
  std::ostream& operator()(int i)
  {
    return os_ << i;
  }
  std::ostream& operator()(double d)
  {
    return os_ << d;
  }
  std::ostream& operator()(const std::string& s)
  {
    return os_ << "'" << s << "'";
  }
  std::ostream& operator()(bool b)
  {
    return os_ << (b ? "true" : "false");
  }
};

}  // namespace warehouse_ros_couchdb

#endif  // WAREHOUSE_ROS_COUCHDB__IMPL__VARIANT_HPP_
