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

#include <warehouse_ros_couchdb/message_collection_helper.hpp>

#include <warehouse_ros_couchdb/exceptions.hpp>
#include <warehouse_ros_couchdb/metadata.hpp>
#include <warehouse_ros_couchdb/query.hpp>

#include <boost/make_shared.hpp>
#include <rclcpp/rclcpp.hpp>

#include <curl/curl.h>
#include <json/json.h>

#include <sstream>
#include <string>
#include <utility>
#include <iomanip>

static const rclcpp::Logger LOGGER = rclcpp::get_logger("warehouse_ros_couchdb.MessageCollectionHelper");

namespace warehouse_ros_couchdb
{

// Helper function to make HTTP requests to CouchDB
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string performHttpRequest(const std::string& url, const std::string& method, const std::string& data = "") {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw InternalError("Failed to initialize CURL", "");
    }
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    
    struct curl_slist* headers = nullptr;
    if (!data.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        if (headers) curl_slist_free_all(headers);
        throw InternalError("HTTP request failed", curl_easy_strerror(res));
    }
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    if (headers) curl_slist_free_all(headers);
    
    return response;
}

MessageCollectionHelper::Md5CompareResult MessageCollectionHelper::findAndMatchMd5Sum(const std::array<unsigned char, 16> & md5_bytes)
{
  try {
    std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/" + db_name_ + "/" + collection_name_ + "/_find";
    
    Json::Value query_doc;
    Json::Value selector;
    
    // Convert MD5 bytes to hex string for storage/comparison
    std::stringstream md5_hex;
    for (const auto& byte : md5_bytes) {
      md5_hex << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
    }
    
    selector["md5sum"] = md5_hex.str();
    query_doc["selector"] = selector;
    query_doc["limit"] = 1;
    
    Json::StreamWriterBuilder builder;
    std::string json_query = Json::writeString(builder, query_doc);
    
    std::string response = performHttpRequest(url, "POST", json_query);
    
    Json::Value result;
    Json::CharReaderBuilder reader_builder;
    std::string errors;
    std::stringstream ss(response);
    
    if (!Json::parseFromStream(reader_builder, ss, &result, &errors)) {
      RCLCPP_ERROR_STREAM(LOGGER, "Failed to parse JSON response: " << errors);
      return Md5CompareResult::EMPTY;
    }
    
    if (result["docs"].empty()) {
      return Md5CompareResult::EMPTY;
    }
    
    // Check if MD5 matches exactly
    if (result["docs"].size() > 0 && result["docs"][0]["md5sum"].asString() == md5_hex.str()) {
      return Md5CompareResult::MATCH;
    }
    
    return Md5CompareResult::MISMATCH;
    
  } catch (const std::exception& e) {
    RCLCPP_ERROR_STREAM(LOGGER, "Error in findAndMatchMd5Sum: " << e.what());
    return Md5CompareResult::EMPTY;
  }
}

bool MessageCollectionHelper::initialize(const std::string & datatype, const std::string & md5)
{
  // For CouchDB, we store metadata about the collection in a metadata document
  try {
    // Check if database exists, create if not
    std::string db_url = "http://" + host_ + ":" + std::to_string(port_) + "/" + db_name_;
    
    try {
      performHttpRequest(db_url, "HEAD");
    } catch (...) {
      // Database doesn't exist, create it
      performHttpRequest(db_url, "PUT");
    }
    
    // Store collection metadata
    std::string metadata_doc_id = collection_name_ + "_metadata";
    std::string metadata_url = db_url + "/" + metadata_doc_id;
    
    Json::Value metadata;
    metadata["_id"] = metadata_doc_id;
    metadata["collection_name"] = collection_name_;
    metadata["datatype"] = datatype;
    metadata["md5sum"] = md5;
    metadata["created_at"] = std::time(nullptr);
    
    Json::StreamWriterBuilder builder;
    std::string json_data = Json::writeString(builder, metadata);
    
    try {
      performHttpRequest(metadata_url, "PUT", json_data);
    } catch (...) {
      // Document might already exist, try to update it
      try {
        std::string existing = performHttpRequest(metadata_url, "GET");
        Json::Value existing_doc;
        Json::CharReaderBuilder reader_builder;
        std::string errors;
        std::stringstream ss(existing);
        
        if (Json::parseFromStream(reader_builder, ss, &existing_doc, &errors)) {
          metadata["_rev"] = existing_doc["_rev"];
          json_data = Json::writeString(builder, metadata);
          performHttpRequest(metadata_url, "PUT", json_data);
        }
      } catch (...) {
        RCLCPP_WARN_STREAM(LOGGER, "Failed to create/update collection metadata for " << collection_name_);
      }
    }
    
    return true;
  } catch (const std::exception& e) {
    RCLCPP_ERROR_STREAM(LOGGER, "Failed to initialize collection " << collection_name_ << ": " << e.what());
    return false;
  }
}

void MessageCollectionHelper::insert(char * msg, size_t msg_size, warehouse_ros::Metadata::ConstPtr metadata)
{
  try {
    std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/" + db_name_;
    
    Json::Value doc;
    
    // Generate a unique document ID
    std::string doc_id = collection_name_ + "_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(rand());
    doc["_id"] = doc_id;
    doc["collection"] = collection_name_;
    
    // Store the message data (base64 encoded binary data)
    std::string encoded_msg;
    // Simple base64-like encoding for binary data storage
    encoded_msg.reserve(msg_size * 4 / 3 + 4);
    for (size_t i = 0; i < msg_size; ++i) {
      encoded_msg += std::to_string(static_cast<unsigned char>(msg[i]));
      if (i < msg_size - 1) encoded_msg += ",";
    }
    doc["message_data"] = encoded_msg;
    doc["message_size"] = static_cast<int>(msg_size);
    
    // Add metadata if provided
    if (metadata) {
      Json::Value meta_json;
      // Convert metadata to JSON using lookupFieldNames()
      auto field_names = metadata->lookupFieldNames();
      for (const auto& field_name : field_names) {
        if (metadata->lookupField(field_name)) {
          // Try to determine the field type and add to JSON
          try {
            // Try string first
            std::string str_val = metadata->lookupString(field_name);
            meta_json[field_name] = str_val;
          } catch (...) {
            try {
              // Try double
              double double_val = metadata->lookupDouble(field_name);
              meta_json[field_name] = double_val;
            } catch (...) {
              try {
                // Try int
                int int_val = metadata->lookupInt(field_name);
                meta_json[field_name] = int_val;
              } catch (...) {
                try {
                  // Try bool
                  bool bool_val = metadata->lookupBool(field_name);
                  meta_json[field_name] = bool_val;
                } catch (...) {
                  // If all else fails, skip this field
                  continue;
                }
              }
            }
          }
        }
      }
      doc["metadata"] = meta_json;
    }
    
    doc["timestamp"] = std::time(nullptr);
    
    Json::StreamWriterBuilder builder;
    std::string json_data = Json::writeString(builder, doc);
    
    std::string response = performHttpRequest(url, "POST", json_data);
    
    RCLCPP_DEBUG_STREAM(LOGGER, "Inserted document into collection " << collection_name_);
    
  } catch (const std::exception& e) {
    throw InternalError("Failed to insert message", e.what());
  }
}

warehouse_ros::ResultIteratorHelper::Ptr MessageCollectionHelper::query(
  warehouse_ros::Query::ConstPtr query, const std::string & sort_by, bool ascending) const
{
  // Suppress unused parameter warnings
  (void)query;
  (void)sort_by;
  (void)ascending;
  
  // For now, return a simple implementation
  // This would need a proper CouchDB result iterator implementation
  RCLCPP_WARN_STREAM(LOGGER, "Query functionality not fully implemented for CouchDB backend");
  return warehouse_ros::ResultIteratorHelper::Ptr();
}

unsigned MessageCollectionHelper::removeMessages(warehouse_ros::Query::ConstPtr query)
{
  // Suppress unused parameter warning
  (void)query;
  
  // For now, return 0 - would need proper CouchDB query implementation
  RCLCPP_WARN_STREAM(LOGGER, "RemoveMessages functionality not fully implemented for CouchDB backend");
  return 0;
}

void MessageCollectionHelper::modifyMetadata(
  warehouse_ros::Query::ConstPtr q,
  warehouse_ros::Metadata::ConstPtr m)
{
  // Suppress unused parameter warnings
  (void)q;
  (void)m;
  
  // For now, do nothing - would need proper CouchDB query implementation
  RCLCPP_WARN_STREAM(LOGGER, "ModifyMetadata functionality not fully implemented for CouchDB backend");
}

unsigned MessageCollectionHelper::count()
{
  try {
    std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/" + db_name_ + "/_find";
    
    Json::Value query_doc;
    Json::Value selector;
    
    selector["collection"] = collection_name_;
    query_doc["selector"] = selector;
    query_doc["limit"] = 0; // Just count, don't return documents
    
    Json::StreamWriterBuilder builder;
    std::string json_query = Json::writeString(builder, query_doc);
    
    std::string response = performHttpRequest(url, "POST", json_query);
    
    Json::Value result;
    Json::CharReaderBuilder reader_builder;
    std::string errors;
    std::stringstream ss(response);
    
    if (!Json::parseFromStream(reader_builder, ss, &result, &errors)) {
      RCLCPP_ERROR_STREAM(LOGGER, "Failed to parse JSON response: " << errors);
      return 0;
    }
    
    return result["docs"].size();
    
  } catch (const std::exception& e) {
    RCLCPP_ERROR_STREAM(LOGGER, "Error counting documents: " << e.what());
    return 0;
  }
}

warehouse_ros::Query::Ptr MessageCollectionHelper::createQuery() const
{
  return boost::make_shared<Query>();
}

warehouse_ros::Metadata::Ptr MessageCollectionHelper::createMetadata() const
{
  return boost::make_shared<Metadata>();
}

}  // namespace warehouse_ros_couchdb
