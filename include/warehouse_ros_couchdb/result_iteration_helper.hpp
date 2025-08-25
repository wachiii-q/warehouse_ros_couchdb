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

#ifndef WAREHOUSE_ROS_COUCHDB__RESULT_ITERATION_HELPER_HPP_
#define WAREHOUSE_ROS_COUCHDB__RESULT_ITERATION_HELPER_HPP_

#include <warehouse_ros_couchdb/warehouse_ros_couchdb_export.hpp>
#include <warehouse_ros/query_results.h>

#include <json/json.h>
#include <string>
#include <vector>

namespace warehouse_ros_couchdb
{
class WAREHOUSE_ROS_COUCHDB_EXPORT ResultIteratorHelper : public warehouse_ros::ResultIteratorHelper
{
  Json::Value results_;
  size_t current_index_;
  
public:
  ResultIteratorHelper() : current_index_(0) {}
  explicit ResultIteratorHelper(const Json::Value& results)
  : results_(results), current_index_(0)
  {
  }
  
  bool next() override;
  bool hasData() const override;
  warehouse_ros::Metadata::ConstPtr metadata() const override;
  std::string message() const override;
};

}  // namespace warehouse_ros_couchdb

#endif  // WAREHOUSE_ROS_COUCHDB__RESULT_ITERATION_HELPER_HPP_
