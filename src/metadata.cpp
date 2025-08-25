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

#include <warehouse_ros_couchdb/metadata.hpp>
#include <warehouse_ros_couchdb/exceptions.hpp>

#include <boost/variant/get.hpp>
#include <stdexcept>
#include <set>

namespace warehouse_ros_couchdb
{

// Helper visitor to handle null values
template<typename R>
class NullValueVisitor : public boost::static_visitor<R>
{
public:
  R operator()(const NullValue&) const {
    throw std::range_error("Null value encountered");
  }
  
  R operator()(const R& value) const {
    return value;
  }
  
  template<typename T>
  R operator()(const T&) const {
    throw std::range_error("Type mismatch");
  }
};

template<typename R>
R Metadata::doLookup(const std::string & name) const
{
  const auto res = data_.find(name);
  if (res == data_.end()) {
    throw std::range_error("Key not found: " + name);
  }
  return boost::apply_visitor(NullValueVisitor<R>(), res->second);
}

void Metadata::append(const std::string & name, const std::string & val)
{
  data_[name] = val;
}

void Metadata::append(const std::string & name, const double val)
{
  data_[name] = val;
}

void Metadata::append(const std::string & name, const int val)
{
  data_[name] = val;
}

void Metadata::append(const std::string & name, const bool val)
{
  data_[name] = val ? 1 : 0;  // Store as int
}

std::string Metadata::lookupString(const std::string & name) const
{
  return doLookup<std::string>(name);
}

double Metadata::lookupDouble(const std::string & name) const
{
  return doLookup<double>(name);
}

int Metadata::lookupInt(const std::string & name) const
{
  return doLookup<int>(name);
}

bool Metadata::lookupBool(const std::string & name) const
{
  return doLookup<int>(name) != 0;
}

bool Metadata::lookupField(const std::string & name) const
{
  return data_.find(name) != data_.end();
}

std::set<std::string> Metadata::lookupFieldNames() const
{
  std::set<std::string> field_names;
  for (const auto& pair : data_) {
    field_names.insert(pair.first);
  }
  return field_names;
}

void Metadata::ensureColumns(const std::string & host, const std::string & collection_name) const
{
  // For CouchDB, we don't need to ensure columns exist like in SQL
  // This is a no-op since CouchDB is schema-less
  (void)host;             // Suppress unused parameter warning
  (void)collection_name;  // Suppress unused parameter warning
}

}  // namespace warehouse_ros_couchdb
