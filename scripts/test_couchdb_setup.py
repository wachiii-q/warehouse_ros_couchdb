#!/usr/bin/env python3
"""
Simple test script for warehouse_ros_couchdb
This script tests basic CouchDB connectivity and operations
"""

import subprocess
import json
import requests
from requests.auth import HTTPBasicAuth
import time
import sys

def test_couchdb_connection():
    """Test basic connection to CouchDB."""
    # Test server info endpoint
    response = requests.get("http://localhost:5984/", auth=HTTPBasicAuth("admin", "12345678"))
    
    if response.status_code == 200:
        server_info = response.json()
        print(f"✅ CouchDB connection successful! Version: {server_info.get('version', 'unknown')}")
        return True
    else:
        print(f"❌ CouchDB connection failed: {response.status_code}")
        return False

def create_test_database():
    """Create a test database"""
    try:
        response = requests.put('http://localhost:5984/test_warehouse_ros', auth=HTTPBasicAuth("admin", "12345678"))
        if response.status_code in [201, 412]:  # 201 = created, 412 = already exists
            print("✅ Test database created/exists")
            return True
        else:
            print(f"❌ Failed to create database: {response.status_code} - {response.text}")
            return False
    except Exception as e:
        print(f"❌ Error creating database: {e}")
        return False

def test_document_operations():
    """Test basic document CRUD operations"""
    try:
        # Create a test document
        doc = {
            "_id": "test_doc_001",
            "collection": "test_collection",
            "message_data": "test_message_data",
            "timestamp": int(time.time())
        }
        
        response = requests.put('http://localhost:5984/test_warehouse_ros/test_doc_001', 
                              json=doc, auth=HTTPBasicAuth("admin", "12345678"))
        if response.status_code in [201, 409]:  # 201 = created, 409 = conflict (already exists)
            print("✅ Document creation test passed")
        else:
            print(f"❌ Document creation failed: {response.status_code}")
            return False
        
        # Read the document
        response = requests.get('http://localhost:5984/test_warehouse_ros/test_doc_001', auth=HTTPBasicAuth("admin", "12345678"))
        if response.status_code == 200:
            print("✅ Document read test passed")
        else:
            print(f"❌ Document read failed: {response.status_code}")
            return False
        
        # Query documents
        query = {
            "selector": {
                "collection": "test_collection"
            }
        }
        response = requests.post('http://localhost:5984/test_warehouse_ros/_find', 
                               json=query, auth=HTTPBasicAuth("admin", "12345678"))
        if response.status_code == 200:
            docs = response.json().get('docs', [])
            print(f"✅ Query test passed - found {len(docs)} documents")
        else:
            print(f"❌ Query test failed: {response.status_code}")
            return False
        
        return True
        
    except Exception as e:
        print(f"❌ Document operations error: {e}")
        return False

def cleanup_test_database():
    """Clean up test database"""
    try:
        response = requests.delete('http://localhost:5984/test_warehouse_ros', auth=HTTPBasicAuth("admin", "12345678"))
        if response.status_code in [200, 404]:  # 200 = deleted, 404 = not found
            print("✅ Test database cleaned up")
        else:
            print(f"⚠️  Database cleanup warning: {response.status_code}")
    except Exception as e:
        print(f"⚠️  Database cleanup error: {e}")

def main():
    print("🧪 Testing warehouse_ros_couchdb setup...\n")
    
    # Test CouchDB connection
    if not test_couchdb_connection():
        print("\n💡 To start CouchDB:")
        print("   sudo systemctl start couchdb")
        print("   OR install with: sudo apt install couchdb")
        return 1
    
    # Test database operations
    if not create_test_database():
        return 1
    
    if not test_document_operations():
        cleanup_test_database()
        return 1
    
    cleanup_test_database()
    
    print("\n🎉 All tests passed! warehouse_ros_couchdb is ready to use.")
    print("\n📋 Next steps:")
    print("   1. Create a ROS2 node that uses warehouse_ros_couchdb")
    print("   2. Test with MoveIt2 or other warehouse_ros applications")
    print("   3. Configure the plugin to use CouchDB parameters:")
    print("      - host: localhost")
    print("      - port: 5984") 
    print("      - database: your_robot_data")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
