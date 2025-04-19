#!/usr/bin/env python3

import os
import subprocess
import time
import signal
import socket
import sys

# Most of the tests written by claude, reviewed by seamar

# Configuration
SERVER_BINARY = "./bin/webserver"
TEST_CONFIG = "test_config.conf"
SERVER_PORT = 8081
SERVER_HOST = "localhost"
TEST_OUTPUT_DIR = "integration_test_output"
WAIT_TIME = 2  # wait time between operations

# Create test directory if it doesn't exist
os.makedirs(TEST_OUTPUT_DIR, exist_ok=True)

# Create test config file
with open(TEST_CONFIG, 'w') as f:
    f.write(f"listen {SERVER_PORT};")

server_process = None

# Helper functions to create various HTTP requests
def createGETRequest(host, name):
    return ("\r\n".join([f"GET /{name} HTTP/1.1", f"Host: {host}", "", ""])).encode()

def createPOSTRequest(host, name):
    return ("\r\n".join([f"POST /{name} HTTP/1.1", f"Host: {host}", "Content-Length: 0", "", ""])).encode()

def run_server():
    """Start the server as a subprocess"""
    global server_process
    print(f"Starting server on port {SERVER_PORT}...")
    # Run server with error output redirected to a file
    error_log = os.path.join(TEST_OUTPUT_DIR, "server_error.log")
    with open(error_log, "w") as err_file:
        server_process = subprocess.Popen(
            [SERVER_BINARY, TEST_CONFIG],
            stderr=err_file,
            stdout=subprocess.PIPE
        )
    
    # Wait for server to start
    time.sleep(WAIT_TIME)
    
    # Check if server is running
    if server_process.poll() is not None:
        print(f"Error: Server failed to start. Exit code: {server_process.returncode}")
        sys.exit(1)
    
    print(f"Server started with PID {server_process.pid}")

def check_server_running():
    """Check if the server is still running, restart if needed"""
    global server_process
    if server_process and server_process.poll() is not None:
        exit_code = server_process.returncode
        print(f"WARNING: Server process exited with code {exit_code}")
        if exit_code < 0:
            signal_name = signal.Signals(-exit_code).name if -exit_code in signal.Signals else "UNKNOWN"
            print(f"Server terminated by signal: {signal_name} ({-exit_code})")
        
        # Clean up any zombie connections before restarting
        try:
            # Try to connect and close quickly to clear any lingering connections
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(0.1)
            sock.connect((SERVER_HOST, SERVER_PORT))
            sock.close()
        except:
            pass
        
        time.sleep(WAIT_TIME)  # Additional delay before restart
        run_server()
        time.sleep(WAIT_TIME)  # Wait after restart
        return True  # Server was restarted
    return False  # No restart needed

def stop_server():
    """Stop the server"""
    global server_process
    if server_process:
        if server_process.poll() is None:  # Only if process is still running
            print(f"Stopping server (PID {server_process.pid})...")
            try:
                server_process.terminate()
                time.sleep(1)
                if server_process.poll() is None:  # Still running after terminate
                    server_process.kill()
                server_process.wait(timeout=2)
            except Exception as e:
                print(f"Error stopping server: {e}")
        print("Server stopped")

def send_request_and_check(test_name, request_data, expected_status, save_response=True):
    """Send a request and check the response"""
    print(f"\nRunning test: {test_name}")
    
    # Check if we need to restart the server
    restarted = check_server_running()
    if restarted:
        print(f"Server was restarted before test: {test_name}")
    
    # Create a new socket for each test
    sock = None
    try:
        sock = socket.create_connection((SERVER_HOST, SERVER_PORT), timeout=5)
        print(f"Connected to server on port {SERVER_PORT}")
        
        # Log the request we're about to send
        print(f"Sending request: {request_data[:50]}...")
        
        sock.sendall(request_data)
        response = sock.recv(4096)
        response_text = response.decode()
        
        # Save response to file if requested
        if save_response:
            output_file = os.path.join(TEST_OUTPUT_DIR, f"{test_name}.txt")
            with open(output_file, 'w') as f:
                f.write(response_text)
        
        # Check if response contains expected status
        if response_text.startswith(expected_status):
            print(f"Test {test_name}: PASSED - Response contains expected status: {expected_status}")
            return True
        else:
            print(f"Test {test_name}: FAILED - Response doesn't contain expected status: {expected_status}")
            print(f"Actual response (first 100 chars):\n{response_text[:100]}")
            return False
    except socket.error as e:
        print(f"Test {test_name}: FAILED - Socket error: {e}")
        return False
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass
        time.sleep(WAIT_TIME)  # Wait before next test

def test_multiple_requests_same_connection():
    """Test sending multiple requests on the same connection"""
    print("\nRunning test: multiple_requests_same_connection")
    
    # Check if we need to restart the server
    restarted = check_server_running()
    if restarted:
        print("Server was restarted before multiple request test")
    
    # Create a single socket for both requests
    sock = None
    try:
        sock = socket.create_connection((SERVER_HOST, SERVER_PORT), timeout=5)
        print(f"Connected to server on port {SERVER_PORT}")
        
        # First request
        first_request = createGETRequest(SERVER_HOST, "first")
        print(f"Sending first request: {first_request[:50]}...")
        sock.sendall(first_request)
        
        # Get response to first request
        first_response = sock.recv(4096)
        first_response_text = first_response.decode()
        
        # Save first response
        with open(os.path.join(TEST_OUTPUT_DIR, "multiple_requests_first.txt"), 'w') as f:
            f.write(first_response_text)
        
        # Check first response
        first_request_passed = first_response_text.startswith("HTTP/1.1 200 OK")
        if first_request_passed:
            print("First request: PASSED - Response contains expected status: HTTP/1.1 200 OK")
        else:
            print("First request: FAILED - Response doesn't contain expected status: HTTP/1.1 200 OK")
            print(f"Actual response (first 100 chars):\n{first_response_text[:100]}")
        
        # Second request on same connection
        second_request = createGETRequest(SERVER_HOST, "second")
        print(f"Sending second request on same connection: {second_request[:50]}...")
        sock.sendall(second_request)
        
        # Get response to second request
        second_response = sock.recv(4096)
        second_response_text = second_response.decode()
        
        # Save second response
        with open(os.path.join(TEST_OUTPUT_DIR, "multiple_requests_second.txt"), 'w') as f:
            f.write(second_response_text)
        
        # Check second response
        second_request_passed = second_response_text.startswith("HTTP/1.1 200 OK")
        if second_request_passed:
            print("Second request: PASSED - Response contains expected status: HTTP/1.1 200 OK")
        else:
            print("Second request: FAILED - Response doesn't contain expected status: HTTP/1.1 200 OK")
            print(f"Actual response (first 100 chars):\n{second_response_text[:100]}")
        
        # Overall test result
        return first_request_passed and second_request_passed
    
    except socket.error as e:
        print(f"Multiple requests test: FAILED - Socket error: {e}")
        return False
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass
        time.sleep(WAIT_TIME)  # Wait before next test
    

def run_all_tests():
    """Run all tests with better resilience to server crashes"""
    test_results = {}
    
    # Basic tests - simple valid requests that should work
    test_results["get_request_hello"] = send_request_and_check(
        "get_request_hello", 
        createGETRequest(SERVER_HOST, "helloimbot"), 
        "HTTP/1.1 200 OK"
    )
    
    # Test for 400 with POST request
    test_results["post_request"] = send_request_and_check(
        "post_request", 
        createPOSTRequest(SERVER_HOST, "test"), 
        "HTTP/1.1 400 Bad Request"
    )

    # Test multiple requests on the same connection
    test_results["multiple_requests_same_connection"] = test_multiple_requests_same_connection()
    
    # Print summary
    print("\n--- Test Results Summary ---")
    all_passed = True
    for test, result in test_results.items():
        print(f"{test}: {'PASSED' if result else 'FAILED'}")
        if not result:
            all_passed = False
    
    return all_passed

def main():
    """Main function"""
    print("Starting integration tests...")
    
    try:
        # Start server
        run_server()
        
        # Run all tests
        tests_passed = run_all_tests()
        
        # Return test status
        if tests_passed:
            print("\nAll integration tests PASSED")
            return 0
        else:
            print("\nSome integration tests FAILED")
            return 1
    except KeyboardInterrupt:
        print("\nIntegration test interrupted. Cleaning up...")
        return 1
    except Exception as e:
        print(f"\nUnexpected error: {e}")
        return 1
    finally:
        # Ensure server is stopped
        stop_server()
        # Remove config file if it exists
        if os.path.exists(TEST_CONFIG):
            os.remove(TEST_CONFIG)

if __name__ == "__main__":
    sys.exit(main())