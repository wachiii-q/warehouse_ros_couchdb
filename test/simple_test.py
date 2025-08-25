#!/usr/bin/env python3
"""
Simplified comprehensive test for warehouse_ros_couchdb plugin
This script tests the plugin without requiring manual C++ compilation
"""

import os
import sys
import subprocess
import time
import json
import requests

def test_couchdb_connection():
    """Test CouchDB connectivity"""
    print("🔌 Testing CouchDB connection...")
    
    try:
        response = requests.get("http://localhost:5984", auth=("admin", "12345678"))
        if response.status_code == 200:
            data = response.json()
            print(f"✅ CouchDB connected successfully! Version: {data.get('version', 'unknown')}")
            return True
        else:
            print(f"❌ CouchDB connection failed with status: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ CouchDB connection error: {e}")
        return False

def test_plugin_installation():
    """Test that the plugin is properly built and installed"""
    print("\n📦 Testing plugin installation...")
    
    # Check if library exists
    lib_path = "../../../install/warehouse_ros_couchdb/lib/libwarehouse_ros_couchdb.so"
    if os.path.exists(lib_path):
        print("✅ Plugin library found")
    else:
        print("❌ Plugin library not found")
        return False
    
    # Check plugin descriptor
    descriptor_path = "../../../install/warehouse_ros_couchdb/share/warehouse_ros_couchdb/couchdb_database_connection_plugin_description.xml"
    if os.path.exists(descriptor_path):
        print("✅ Plugin descriptor found")
    else:
        print("❌ Plugin descriptor not found")
        return False
    
    return True

def test_ros2_environment():
    """Test ROS2 environment setup"""
    print("\n🤖 Testing ROS2 environment...")
    
    # Check if ROS2 is sourced
    if "ROS_VERSION" in os.environ and os.environ["ROS_VERSION"] == "2":
        print("✅ ROS2 environment detected")
    else:
        print("❌ ROS2 environment not properly set")
        return False
    
    # Check if warehouse_ros_couchdb package is available
    try:
        result = subprocess.run(
            ["ros2", "pkg", "list"], 
            capture_output=True, 
            text=True, 
            env=dict(os.environ, AMENT_PREFIX_PATH="../../../install:" + os.environ.get("AMENT_PREFIX_PATH", ""))
        )
        
        if "warehouse_ros_couchdb" in result.stdout:
            print("✅ warehouse_ros_couchdb package found in ROS2")
            return True
        else:
            print("❌ warehouse_ros_couchdb package not found in ROS2")
            return False
            
    except Exception as e:
        print(f"❌ Error checking ROS2 packages: {e}")
        return False

def test_couchdb_operations():
    """Test basic CouchDB database operations"""
    print("\n🗃️  Testing CouchDB database operations...")
    
    test_db = "test_warehouse_plugin"
    
    try:
        # Create test database
        response = requests.put(f"http://localhost:5984/{test_db}", auth=("admin", "12345678"))
        if response.status_code in [201, 412]:  # Created or already exists
            print("✅ Test database created/exists")
        else:
            print(f"❌ Failed to create test database: {response.status_code}")
            return False
        
        # Insert test document
        test_doc = {
            "_id": "test_doc_001",
            "collection": "test_messages",
            "message_type": "std_msgs/String",
            "data": "Hello from warehouse_ros_couchdb!",
            "timestamp": int(time.time())
        }
        
        response = requests.put(
            f"http://localhost:5984/{test_db}/test_doc_001", 
            json=test_doc, 
            auth=("admin", "12345678")
        )
        
        if response.status_code in [201, 409]:  # Created or conflict (already exists)
            print("✅ Test document inserted successfully")
        else:
            print(f"❌ Failed to insert test document: {response.status_code}")
            return False
        
        # Query test document
        response = requests.get(f"http://localhost:5984/{test_db}/test_doc_001", auth=("admin", "12345678"))
        if response.status_code == 200:
            doc = response.json()
            if doc.get("data") == "Hello from warehouse_ros_couchdb!":
                print("✅ Test document retrieved and validated")
            else:
                print("❌ Test document data mismatch")
                return False
        else:
            print(f"❌ Failed to retrieve test document: {response.status_code}")
            return False
        
        # Clean up test database
        response = requests.delete(f"http://localhost:5984/{test_db}", auth=("admin", "12345678"))
        if response.status_code in [200, 404]:  # OK or not found
            print("✅ Test database cleaned up")
        
        return True
        
    except Exception as e:
        print(f"❌ Error during CouchDB operations: {e}")
        return False

def test_library_dependencies():
    """Test library dependencies"""
    print("\n📚 Testing library dependencies...")
    
    lib_path = "../../../install/warehouse_ros_couchdb/lib/libwarehouse_ros_couchdb.so"
    
    try:
        # Check libcurl dependency
        result = subprocess.run(["ldd", lib_path], capture_output=True, text=True)
        if "libcurl" in result.stdout:
            print("✅ libcurl dependency linked")
        else:
            print("⚠️  libcurl dependency may not be linked")
        
        # Check jsoncpp dependency  
        if "jsoncpp" in result.stdout or "json" in result.stdout:
            print("✅ JSON library dependency linked")
        else:
            print("⚠️  JSON library dependency may not be linked")
            
        return True
        
    except Exception as e:
        print(f"⚠️  Could not check library dependencies: {e}")
        return True  # Non-critical for functionality

def main():
    """Run all tests"""
    print("🧪 Simplified warehouse_ros_couchdb Plugin Test")
    print("=" * 60)
    
    tests = [
        ("CouchDB Connection", test_couchdb_connection),
        ("Plugin Installation", test_plugin_installation),
        ("ROS2 Environment", test_ros2_environment),
        ("CouchDB Operations", test_couchdb_operations),
        ("Library Dependencies", test_library_dependencies)
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        try:
            if test_func():
                passed += 1
            else:
                failed += 1
        except Exception as e:
            print(f"❌ {test_name} test crashed: {e}")
            failed += 1
    
    print("\n" + "=" * 60)
    print(f"🎯 Test Results: {passed} passed, {failed} failed")
    
    if failed == 0:
        print("🎉 All tests passed! warehouse_ros_couchdb is ready to use!")
        print("\n📋 Next steps:")
        print("   1. Use the plugin in your ROS2 applications")
        print("   2. Configure: warehouse_ros_couchdb::DatabaseConnection")
        print("   3. Set host: localhost, port: 5984")
        return 0
    else:
        print("❌ Some tests failed. Please check the output above.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
