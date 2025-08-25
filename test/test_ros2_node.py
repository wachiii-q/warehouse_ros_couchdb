#!/usr/bin/env python3
"""
Simple ROS2 node to test warehouse_ros_couchdb integration
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import requests
from requests.auth import HTTPBasicAuth
import json
import time

class WarehouseCouchDBTestNode(Node):
    def __init__(self):
        super().__init__('warehouse_couchdb_test_node')
        
        # CouchDB connection parameters
        self.couchdb_host = 'localhost'
        self.couchdb_port = 5984
        self.database_name = 'ros2_test_warehouse'
        self.couchdb_auth = HTTPBasicAuth('admin', '12345678')
        
        self.get_logger().info('🧪 Starting warehouse_ros_couchdb test node...')
        
        # Test CouchDB connectivity
        self.test_couchdb_connection()
        
        # Create test publisher and subscriber
        self.publisher = self.create_publisher(String, 'test_topic', 10)
        self.subscription = self.create_subscription(
            String, 'test_topic', self.message_callback, 10)
        
        # Timer to send test messages
        self.timer = self.create_timer(5.0, self.publish_test_message)
        self.message_count = 0
        
    def test_couchdb_connection(self):
        """Test basic CouchDB connectivity"""
        try:
            url = f'http://{self.couchdb_host}:{self.couchdb_port}'
            response = requests.get(url, auth=self.couchdb_auth, timeout=5)
            
            if response.status_code == 200:
                info = response.json()
                self.get_logger().info(f'✅ Connected to CouchDB {info.get("version", "unknown")}')
                
                # Create test database
                db_url = f'{url}/{self.database_name}'
                db_response = requests.put(db_url, auth=self.couchdb_auth)
                if db_response.status_code in [201, 412]:  # Created or already exists
                    self.get_logger().info(f'✅ Database "{self.database_name}" ready')
                else:
                    self.get_logger().error(f'❌ Failed to create database: {db_response.status_code}')
                    
            else:
                self.get_logger().error(f'❌ CouchDB returned status: {response.status_code}')
                
        except requests.exceptions.ConnectionError:
            self.get_logger().error('❌ Could not connect to CouchDB. Is it running?')
            self.get_logger().info('💡 Start with: sudo systemctl start couchdb')
        except Exception as e:
            self.get_logger().error(f'❌ CouchDB connection error: {e}')
    
    def publish_test_message(self):
        """Publish a test message and store it in CouchDB"""
        self.message_count += 1
        
        msg = String()
        msg.data = f'Test message #{self.message_count} at {time.time()}'
        
        # Publish ROS message
        self.publisher.publish(msg)
        self.get_logger().info(f'📤 Published: "{msg.data}"')
        
        # Store in CouchDB (simulating warehouse_ros behavior)
        self.store_message_in_couchdb(msg)
    
    def message_callback(self, msg):
        """Handle received messages"""
        self.get_logger().info(f'📥 Received: "{msg.data}"')
    
    def store_message_in_couchdb(self, msg):
        """Store message in CouchDB (simulating warehouse_ros_couchdb plugin behavior)"""
        try:
            doc = {
                '_id': f'msg_{self.message_count}_{int(time.time())}',
                'collection': 'test_messages',
                'message_type': 'std_msgs/String',
                'message_data': msg.data,
                'timestamp': time.time(),
                'metadata': {
                    'node_name': self.get_name(),
                    'topic_name': 'test_topic',
                    'sequence': self.message_count
                }
            }
            
            url = f'http://{self.couchdb_host}:{self.couchdb_port}/{self.database_name}'
            response = requests.post(url, json=doc, auth=self.couchdb_auth, timeout=5)
            
            if response.status_code == 201:
                self.get_logger().info(f'💾 Stored message in CouchDB: {doc["_id"]}')
            else:
                self.get_logger().warning(f'⚠️  CouchDB storage failed: {response.status_code}')
                
        except Exception as e:
            self.get_logger().error(f'❌ Error storing in CouchDB: {e}')
    
    def query_stored_messages(self):
        """Query stored messages from CouchDB"""
        try:
            query = {
                'selector': {
                    'collection': 'test_messages'
                },
                'limit': 10
            }
            
            url = f'http://{self.couchdb_host}:{self.couchdb_port}/{self.database_name}/_find'
            response = requests.post(url, json=query, auth=self.couchdb_auth, timeout=5)
            
            if response.status_code == 200:
                docs = response.json().get('docs', [])
                self.get_logger().info(f'🔍 Found {len(docs)} stored messages')
                
                for doc in docs[-3:]:  # Show last 3 messages
                    self.get_logger().info(f'   📋 {doc["_id"]}: {doc["message_data"]}')
            else:
                self.get_logger().warning(f'⚠️  Query failed: {response.status_code}')
                
        except Exception as e:
            self.get_logger().error(f'❌ Query error: {e}')

def main(args=None):
    rclpy.init(args=args)
    
    node = WarehouseCouchDBTestNode()
    
    try:
        # Query existing messages after a short delay
        node.create_timer(10.0, node.query_stored_messages)
        
        rclpy.spin(node)
        
    except KeyboardInterrupt:
        node.get_logger().info('🛑 Shutting down warehouse_couchdb_test_node...')
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
