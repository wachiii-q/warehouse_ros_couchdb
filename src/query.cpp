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

#include <warehouse_ros_couchdb/query.hpp>

#include <rclcpp/rclcpp.hpp>

#include <json/json.h>
#include <sstream>
#include <string>

static const rclcpp::Logger LOGGER = rclcpp::get_logger("warehouse_ros_couchdb.query");

namespace warehouse_ros_couchdb
{

void Query::append(const std::string & name, const std::string & val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " = '" << val << "'";
}

void Query::append(const std::string & name, const double val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " = " << val;
}

void Query::append(const std::string & name, const int val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " = " << val;
}

void Query::append(const std::string & name, const bool val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val ? 1 : 0);
  query_ << name << " = " << (val ? "true" : "false");
}

void Query::appendLT(const std::string & name, const double val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " < " << val;
}

void Query::appendLT(const std::string & name, const int val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " < " << val;
}

void Query::appendLTE(const std::string & name, const double val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " <= " << val;
}

void Query::appendLTE(const std::string & name, const int val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " <= " << val;
}

void Query::appendGT(const std::string & name, const double val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " > " << val;
}

void Query::appendGT(const std::string & name, const int val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " > " << val;
}

void Query::appendGTE(const std::string & name, const double val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " >= " << val;
}

void Query::appendGTE(const std::string & name, const int val)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(val);
  query_ << name << " >= " << val;
}

void Query::appendRange(const std::string & name, const double lower, const double upper)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(lower);
  values_.emplace_back(upper);
  query_ << name << " > " << lower << " AND " << name << " < " << upper;
}

void Query::appendRange(const std::string & name, const int lower, const int upper)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(lower);
  values_.emplace_back(upper);
  query_ << name << " > " << lower << " AND " << name << " < " << upper;
}

void Query::appendRangeInclusive(const std::string & name, const double lower, const double upper)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(lower);
  values_.emplace_back(upper);
  query_ << name << " >= " << lower << " AND " << name << " <= " << upper;
}

void Query::appendRangeInclusive(const std::string & name, const int lower, const int upper)
{
  if (!values_.empty()) {
    query_ << " AND ";
  }
  values_.emplace_back(lower);
  values_.emplace_back(upper);
  query_ << name << " >= " << lower << " AND " << name << " <= " << upper;
}

std::string Query::buildCouchDbQuery(const std::string & base_query) const
{
  // Suppress unused parameter warning
  (void)base_query;
  
  // For CouchDB, we would build a JSON query using CouchDB's Mango query language
  // This is a simplified implementation
  Json::Value selector;
  
  // For now, just return a basic selector
  // In a full implementation, we'd parse the query_ stringstream and convert to JSON
  if (!empty()) {
    RCLCPP_WARN_STREAM(LOGGER, "Complex queries not fully implemented for CouchDB backend");
  }
  
  Json::StreamWriterBuilder builder;
  return Json::writeString(builder, selector);
}

}  // namespace warehouse_ros_couchdb
