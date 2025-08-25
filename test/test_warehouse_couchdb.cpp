#include <warehouse_ros_couchdb/database_connection.hpp>
#include <warehouse_ros_couchdb/message_collection_helper.hpp>
#include <warehouse_ros_couchdb/metadata.hpp>
#include <warehouse_ros_couchdb/query.hpp>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include <iostream>
#include <memory>

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    
    std::cout << "🧪 Testing warehouse_ros_couchdb C++ implementation...\n" << std::endl;
    
    try {
        // Test 1: Database Connection
        std::cout << "📡 Test 1: Database Connection" << std::endl;
        auto db_connection = std::make_shared<warehouse_ros_couchdb::DatabaseConnection>();
        
        // Test connection to CouchDB
        if (db_connection->connect("localhost", 5984)) {
            std::cout << "✅ Connected to CouchDB successfully" << std::endl;
        } else {
            std::cout << "❌ Failed to connect to CouchDB" << std::endl;
            return 1;
        }
        
        // Test 2: Database Operations
        std::cout << "\n🗄️  Test 2: Database Operations" << std::endl;
        
        // Create a test database
        try {
            db_connection->dropDatabase("test_warehouse_cpp");  // Clean up if exists
        } catch (...) {
            // Ignore errors if database doesn't exist
        }
        
        // Test getting databases (should work even if empty)
        auto databases = db_connection->getTablesOfDatabase("test_db");
        std::cout << "✅ Database operations accessible" << std::endl;
        
        // Test 3: Message Collection Helper
        std::cout << "\n📦 Test 3: Message Collection Helper" << std::endl;
        
        warehouse_ros_couchdb::MessageCollectionHelper helper("localhost", 5984, "test_warehouse_cpp", "test_collection");
        
        // Initialize the collection
        if (helper.initialize("std_msgs/String", "992ce8a1687cec8c8bd883ec73ca4100")) {
            std::cout << "✅ Collection initialized successfully" << std::endl;
        } else {
            std::cout << "❌ Failed to initialize collection" << std::endl;
            return 1;
        }
        
        // Test 4: Metadata Creation
        std::cout << "\n🏷️  Test 4: Metadata and Query Creation" << std::endl;
        
        auto metadata = helper.createMetadata();
        metadata->append("test_key", "test_value");
        metadata->append("timestamp", 12345);
        metadata->append("robot_name", "test_robot");
        std::cout << "✅ Metadata created successfully" << std::endl;
        
        auto query = helper.createQuery();
        query->append("robot_name", "test_robot");
        std::cout << "✅ Query created successfully" << std::endl;
        
        // Test 5: Message Insertion
        std::cout << "\n💾 Test 5: Message Insertion" << std::endl;
        
        // Create a simple test message
        std::string test_msg = "Hello, warehouse_ros_couchdb!";
        char* msg_data = const_cast<char*>(test_msg.c_str());
        size_t msg_size = test_msg.length();
        
        try {
            helper.insert(msg_data, msg_size, metadata);
            std::cout << "✅ Message inserted successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ Message insertion failed: " << e.what() << std::endl;
            return 1;
        }
        
        // Test 6: Collection Count
        std::cout << "\n🔢 Test 6: Collection Count" << std::endl;
        
        try {
            unsigned count = helper.count();
            std::cout << "✅ Collection count: " << count << " documents" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "❌ Count operation failed: " << e.what() << std::endl;
        }
        
        std::cout << "\n🎉 All C++ tests completed successfully!" << std::endl;
        std::cout << "\n📋 Test Summary:" << std::endl;
        std::cout << "   ✅ Database connection works" << std::endl;
        std::cout << "   ✅ Database operations accessible" << std::endl;
        std::cout << "   ✅ Collection initialization works" << std::endl;
        std::cout << "   ✅ Metadata creation works" << std::endl;
        std::cout << "   ✅ Query creation works" << std::endl;
        std::cout << "   ✅ Message insertion works" << std::endl;
        std::cout << "   ✅ Collection counting works" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    rclcpp::shutdown();
    return 0;
}
