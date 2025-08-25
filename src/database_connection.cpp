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

#include <warehouse_ros_couchdb/database_connection.hpp>

#include <warehouse_ros_couchdb/exceptions.hpp>
#include <warehouse_ros_couchdb/message_collection_helper.hpp>
#include <warehouse_ros_couchdb/utils.hpp>

#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <pluginlib/class_list_macros.hpp>
#include <rclcpp/rclcpp.hpp>

#include <curl/curl.h>
#include <json/json.h>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
const rclcpp::Logger LOGGER = rclcpp::get_logger("warehouse_ros_couchdb.database_connection");

// Callback function to write HTTP response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
  userp->append((char*)contents, size * nmemb);
  return size * nmemb;
}
}  // namespace

/// Setup the database connection. This call assumes setParams() has been previously called.
/// Returns true if the connection was succesfully established.
bool warehouse_ros_couchdb::DatabaseConnection::connect()
{
  // Initialize CURL
  curl_global_init(CURL_GLOBAL_DEFAULT);
  
  // Test connection to CouchDB by getting server info
  std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/";
  std::string response = performHttpRequest("GET", url);
  
  // Check if we got a valid CouchDB response
  return !response.empty() && response.find("couchdb") != std::string::npos;
}

/// Returns whether the database is connected.
bool warehouse_ros_couchdb::DatabaseConnection::isConnected()
{
  // Test connection by trying to get server info
  std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/";
  std::string response = performHttpRequest("GET", url);
  return !response.empty();
}

// HTTP request implementation using libcurl
std::string warehouse_ros_couchdb::DatabaseConnection::performHttpRequest(
    const std::string& method, const std::string& url, const std::string& data) 
{
  CURL* curl;
  CURLcode res;
  std::string response_data;

  curl = curl_easy_init();
  if(curl) {
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Add authentication
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "admin:12345678");
    
    // Set callback to capture response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // Set method and data
    if (method == "POST") {
      curl_easy_setopt(curl, CURLOPT_POST, 1L);
      if (!data.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
      }
    } else if (method == "PUT") {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
      if (!data.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
      }
    } else if (method == "DELETE") {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    // GET is default
    
    // Perform request
    res = curl_easy_perform(curl);
    (void)res;  // Suppress unused variable warning
    
    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
  
  return response_data;
}

std::vector<std::string> warehouse_ros_couchdb::DatabaseConnection::getTablesOfDatabase(
  const std::string & db_name)
{
  // In CouchDB, get all documents from the metadata database that belong to this db_name
  (void)db_name;  // Suppress unused parameter warning for now
  std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/_warehouse_metadata/_all_docs";
  std::string response = performHttpRequest("GET", url);
  
  std::vector<std::string> tables;
  // TODO: Parse JSON response and extract collection names for the specified db_name
  // For now, return empty vector - this will be implemented with proper JSON parsing
  return tables;
}

/// \brief Drop a db and all its collections.
/// A DbClientConnection exception will be thrown if the database is not connected.
void warehouse_ros_couchdb::DatabaseConnection::dropDatabase(const std::string & db_name)
{
  // In CouchDB, delete the database entirely
  std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/" + db_name;
  std::string response = performHttpRequest("DELETE", url);
  
  // Also clean up metadata entries for this database
  std::string metadata_url = "http://" + host_ + ":" + std::to_string(port_) + "/_warehouse_metadata";
  // TODO: Query and delete all metadata documents related to this db_name
}

/// \brief Return the ROS Message type of a given collection
std::string warehouse_ros_couchdb::DatabaseConnection::messageType(
  const std::string & db_name,
  const std::string & collection_name)
{
  // Query metadata database for the collection's message type
  std::string metadata_doc_id = db_name + "_" + collection_name;
  std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/_warehouse_metadata/" + metadata_doc_id;
  std::string response = performHttpRequest("GET", url);
  
  // TODO: Parse JSON response to extract message type
  // For now, return empty string - this will be implemented with proper JSON parsing
  return "";
}

void warehouse_ros_couchdb::DatabaseConnection::initDb()
{
  // Create the metadata database if it doesn't exist
  std::string url = "http://" + host_ + ":" + std::to_string(port_) + "/_warehouse_metadata";
  std::string response = performHttpRequest("PUT", url);
  
  // CouchDB creates databases on-demand, so this is mainly for metadata tracking
  RCLCPP_DEBUG_STREAM(LOGGER, "CouchDB metadata database initialized");
}

bool warehouse_ros_couchdb::DatabaseConnection::schemaVersionSet()
{
  // For CouchDB implementation, we'll always return true since CouchDB handles schema evolution
  // In future versions, we could store version info in a special document
  return true;
}

warehouse_ros::MessageCollectionHelper::Ptr
warehouse_ros_couchdb::DatabaseConnection::openCollectionHelper(
  const std::string & db_name,
  const std::string & collection_name)
{
  return boost::make_shared<warehouse_ros_couchdb::MessageCollectionHelper>(
    host_, port_, db_name, collection_name);
}

warehouse_ros_couchdb::InternalError::InternalError(const char * msg, const std::string& details)
: warehouse_ros::WarehouseRosException(boost::format("%1% %2%") % msg % details)
{
}

PLUGINLIB_EXPORT_CLASS(warehouse_ros_couchdb::DatabaseConnection, warehouse_ros::DatabaseConnection)
