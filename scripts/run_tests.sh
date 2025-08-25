#!/bin/bash

# Test script for warehouse_ros_couchdb
echo "🧪 warehouse_ros_couchdb Test Suite"
echo "================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    if [ "$1" == "SUCCESS" ]; then
        echo -e "${GREEN}✅ $2${NC}"
    elif [ "$1" == "ERROR" ]; then
        echo -e "${RED}❌ $2${NC}"
    elif [ "$1" == "WARNING" ]; then
        echo -e "${YELLOW}⚠️  $2${NC}"
    elif [ "$1" == "INFO" ]; then
        echo -e "${BLUE}ℹ️  $2${NC}"
    fi
}

# Step 1: Check if CouchDB is installed and running
echo -e "\n${BLUE}Step 1: Checking CouchDB Installation${NC}"
if command -v couchdb &> /dev/null; then
    print_status "SUCCESS" "CouchDB is installed"
    
    # Check if CouchDB is running
    if systemctl is-active --quiet couchdb; then
        print_status "SUCCESS" "CouchDB service is running"
    else
        print_status "WARNING" "CouchDB service is not running, attempting to start..."
        sudo systemctl start couchdb
        sleep 3
        if systemctl is-active --quiet couchdb; then
            print_status "SUCCESS" "CouchDB service started successfully"
        else
            print_status "ERROR" "Failed to start CouchDB service"
            exit 1
        fi
    fi
else
    print_status "WARNING" "CouchDB not found, installing..."
    # Try to install CouchDB
    if command -v apt &> /dev/null; then
        sudo apt update
        sudo apt install -y couchdb
    elif command -v yum &> /dev/null; then
        sudo yum install -y couchdb
    else
        print_status "ERROR" "Package manager not found. Please install CouchDB manually."
        exit 1
    fi
fi

# Step 2: Test CouchDB HTTP API
echo -e "\n${BLUE}Step 2: Testing CouchDB HTTP API${NC}"
if curl -s http://localhost:5984 > /dev/null; then
    print_status "SUCCESS" "CouchDB HTTP API is accessible"
    
    # Get CouchDB version info
    VERSION=$(curl -s http://localhost:5984 | python3 -c "import sys, json; print(json.load(sys.stdin)['version'])" 2>/dev/null)
    if [ ! -z "$VERSION" ]; then
        print_status "INFO" "CouchDB Version: $VERSION"
    fi
else
    print_status "ERROR" "CouchDB HTTP API is not accessible on localhost:5984"
    print_status "INFO" "Try: sudo systemctl start couchdb"
    exit 1
fi

# Step 3: Test Python connectivity (if requests is available)
echo -e "\n${BLUE}Step 3: Testing Python Connectivity${NC}"
if python3 -c "import requests" 2>/dev/null; then
    print_status "SUCCESS" "Python requests module available"
    
    # Run Python test
    if [ -f "test_couchdb_setup.py" ]; then
        print_status "INFO" "Running Python CouchDB tests..."
        python3 test_couchdb_setup.py
    else
        print_status "WARNING" "Python test file not found"
    fi
else
    print_status "WARNING" "Python requests module not available"
    print_status "INFO" "Install with: pip3 install requests"
fi

# Step 4: Check warehouse_ros_couchdb build
echo -e "\n${BLUE}Step 4: Checking warehouse_ros_couchdb Build${NC}"
if [ -f "install/warehouse_ros_couchdb/lib/libwarehouse_ros_couchdb.so" ]; then
    print_status "SUCCESS" "warehouse_ros_couchdb library found"
    
    # Check library dependencies
    if ldd install/warehouse_ros_couchdb/lib/libwarehouse_ros_couchdb.so | grep -q "curl"; then
        print_status "SUCCESS" "libcurl dependency linked"
    else
        print_status "WARNING" "libcurl dependency may not be properly linked"
    fi
    
    if ldd install/warehouse_ros_couchdb/lib/libwarehouse_ros_couchdb.so | grep -q "jsoncpp"; then
        print_status "SUCCESS" "jsoncpp dependency linked"
    else
        print_status "WARNING" "jsoncpp dependency may not be properly linked"
    fi
    
else
    print_status "ERROR" "warehouse_ros_couchdb library not found"
    print_status "INFO" "Build with: colcon build --packages-select warehouse_ros_couchdb"
    exit 1
fi

# Step 5: Source the ROS environment and test
echo -e "\n${BLUE}Step 5: Testing ROS2 Integration${NC}"
if [ -f "install/setup.bash" ]; then
    print_status "INFO" "Sourcing ROS2 environment..."
    source install/setup.bash
    
    # Check if the plugin is loadable
    print_status "INFO" "Checking plugin availability..."
    if [ -f "install/warehouse_ros_couchdb/share/warehouse_ros_couchdb/couchdb_database_connection_plugin_description.xml" ]; then
        print_status "SUCCESS" "Plugin descriptor file found"
    else
        print_status "WARNING" "Plugin descriptor file not found"
    fi
    
else
    print_status "WARNING" "ROS2 install/setup.bash not found"
fi

# Step 6: Basic functionality test using curl
echo -e "\n${BLUE}Step 6: Basic CouchDB Operations Test${NC}"
print_status "INFO" "Testing basic CouchDB operations..."

# Create test database
TEST_DB="test_warehouse_$(date +%s)"
if curl -s -u "admin:12345678" -X PUT "http://localhost:5984/$TEST_DB" | grep -q "ok"; then
    print_status "SUCCESS" "Test database created"
    
    # Insert test document
    TEST_DOC='{"_id":"test_doc","collection":"test_msgs","data":"test_data","timestamp":'$(date +%s)'}'
    if curl -s -u "admin:12345678" -X PUT "http://localhost:5984/$TEST_DB/test_doc" -H "Content-Type: application/json" -d "$TEST_DOC" | grep -q "ok"; then
        print_status "SUCCESS" "Test document inserted"
        
        # Query test document
        if curl -s -u "admin:12345678" "http://localhost:5984/$TEST_DB/test_doc" | grep -q "test_data"; then
            print_status "SUCCESS" "Test document retrieved"
        else
            print_status "ERROR" "Failed to retrieve test document"
        fi
    else
        print_status "ERROR" "Failed to insert test document"
    fi
    
    # Clean up test database
    curl -s -u "admin:12345678" -X DELETE "http://localhost:5984/$TEST_DB" > /dev/null
    print_status "INFO" "Test database cleaned up"
else
    print_status "ERROR" "Failed to create test database"
fi

# Summary
echo -e "\n${BLUE}🎯 Test Summary${NC}"
echo "================================="
print_status "INFO" "warehouse_ros_couchdb is ready for testing!"
echo
echo "📋 Next steps to test with real ROS2 applications:"
echo "   1. Create a simple ROS2 node that uses warehouse_ros"
echo "   2. Configure it to use the CouchDB plugin"
echo "   3. Test with MoveIt2 planning scene storage"
echo
echo "🔧 Configuration example:"
echo "   database_plugin: warehouse_ros_couchdb::DatabaseConnection"
echo "   database_host: localhost"
echo "   database_port: 5984"
echo "   database_name: moveit_warehouse"
echo
echo "📖 For more details, see:"
echo "   - ROS2 warehouse_ros documentation"
echo "   - MoveIt2 warehouse configuration"
echo "   - CouchDB HTTP API documentation"
