#!/usr/bin/env python3

import unittest
import subprocess
import time
import logging
import http.client
import os
import socket
from logging.handlers import RotatingFileHandler

# use this variable as root for what is in tests/common_files
test_data_dir = os.environ.get("TEST_DATA_DIR")


class HTTPServerTestCase(unittest.TestCase):
    """
    HTTP server testing framework using unittest with log rotation

    Usage:
    - Subclass HTTPServerTestCase and set class attributes:
        - SERVER_CMD: list of command and args to start server
        - SERVER_CWD: optional cwd
        - HOST: server host (default 'localhost')
        - PORT: server port (int)
        - STDERR_FILE: path to write stderr log
        - STARTUP_DELAY: seconds to wait after starting server
        - TEST_LOG_MAX_BYTES, TEST_LOG_BACKUP_COUNT: rotate test framework log
        - STDERR_MAX_BYTES, STDERR_BACKUP_COUNT: rotate stderr log

    Example:
    ```
    class MyTests(HTTPServerTestCase):
        SERVER_CMD        = ['python', '-m', 'myserver', '--config', 'config.yaml']
        SERVER_CWD        = '/path/to/server'
        HOST              = '127.0.0.1'
        PORT              = 8080
        STDERR_FILE       = 'logs/server_stderr.log'
        STARTUP_DELAY     = 0.5
        TEST_LOG_MAX_BYTES = 5 * 1024 * 1024      # 5 MB
        TEST_LOG_BACKUP_COUNT = 3
        STDERR_MAX_BYTES  = 10 * 1024 * 1024     # 10 MB
        STDERR_BACKUP_COUNT = 2

        def test_health(self):
            status, reason, headers, body = self.send_request('GET', '/health')
            self.assertEqual(status, 200)
    ```
    """

    # Must be overridden in subclass
    SERVER_CMD = None  # e.g. ['python', '-m', 'myserver']
    SERVER_CWD = None  # optional working directory
    HOST = "localhost"
    PORT = None  # must be set to server port
    STDERR_FILE = "server_stderr.log"
    TESTLOG_FILE = "server_test.log"
    STARTUP_DELAY = 0.5  # seconds to wait for server to start
    TERMINATION_TIMEOUT = 5  # seconds to wait for graceful shutdown

    # Log rotation settings
    TEST_LOG_MAX_BYTES = 10 * 1024 * 1024  # 10 MB
    TEST_LOG_BACKUP_COUNT = 5
    STDERR_MAX_BYTES = 10 * 1024 * 1024  # 10 MB
    STDERR_BACKUP_COUNT = 5

    @classmethod
    def restart_server(cls):
        """Stop any running server process and start a new one."""
        cls._logger.info("Restarting server")
        cls._stop_server()
        cls._start_server()

    @classmethod
    def _configure_logging(cls):
        """Configure stderr and unit testing logging"""
        # uses the class name to create a tag for the logger
        cls._logger = logging.getLogger(cls.__name__)
        if not cls._logger.handlers:
            # uses provided path to create a directory (only if needed)
            # for example you may configure `log.txt` in which case no directories are created
            # if you configure `./logs/log.txt` then it creates the `log` directory
            os.makedirs(os.path.dirname(cls.TESTLOG_FILE) or ".", exist_ok=True)
            # this is part of the standard library to automatically rotate log files
            handler = RotatingFileHandler(
                cls.TESTLOG_FILE,
                maxBytes=cls.TEST_LOG_MAX_BYTES,
                backupCount=cls.TEST_LOG_BACKUP_COUNT,
            )
            # sets the format for the logger
            # more information here: https://docs.python.org/3/library/logging.html#logrecord-attributes
            formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
            handler.setFormatter(formatter)
            # add the log rotatting handler to the logger
            cls._logger.addHandler(handler)
            cls._logger.setLevel(logging.DEBUG)

    @classmethod
    def _rotate_stderr(cls):
        """Rotate log files if needed"""
        # Ensure directory exists
        dirpath = os.path.dirname(cls.STDERR_FILE)
        if dirpath:
            os.makedirs(dirpath, exist_ok=True)
        # Rotate if too large
        if (
            os.path.exists(cls.STDERR_FILE)
            and os.path.getsize(cls.STDERR_FILE) >= cls.STDERR_MAX_BYTES
        ):
            # Shift backups: .n to .n+1
            for i in range(cls.STDERR_BACKUP_COUNT - 1, 0, -1):
                src = f"{cls.STDERR_FILE}.{i}"
                dst = f"{cls.STDERR_FILE}.{i+1}"
                if os.path.exists(src):
                    os.replace(src, dst)
            # Original to .1
            os.replace(cls.STDERR_FILE, f"{cls.STDERR_FILE}.1")

    @classmethod
    def _start_server(cls):
        """Launch and setup server with provided settings"""
        if cls.SERVER_CMD is None or cls.PORT is None:
            raise ValueError("SERVER_CMD and PORT must be set in subclass")
        cls._logger.info(f"Starting server: {cls.SERVER_CMD}, cwd={cls.SERVER_CWD}")
        # Rotate stderr log if needed
        cls._rotate_stderr()
        # open file and append to log (a flag)
        stderr_handle = open(cls.STDERR_FILE, "a")
        cls._stderr_handle = stderr_handle
        cls.process = subprocess.Popen(
            cls.SERVER_CMD,
            cwd=cls.SERVER_CWD,
            stdout=subprocess.DEVNULL,
            stderr=stderr_handle,
        )
        time.sleep(cls.STARTUP_DELAY)
        if cls.process.poll() is not None:
            raise RuntimeError(
                f"Server failed to start; exit code {cls.process.returncode}"
            )

    @classmethod
    def _stop_server(cls):
        """Terminate server safely or kill process if it refuses"""
        proc = getattr(cls, "process", None)
        if not proc:
            return
        if proc.poll() is None:
            cls._logger.info("Terminating server")
            proc.terminate()
            try:
                proc.wait(timeout=cls.TERMINATION_TIMEOUT)
                cls._logger.info("Server terminated gracefully")
            except subprocess.TimeoutExpired:
                cls._logger.warning("Server did not terminate; killing")
                proc.kill()
        cls._stderr_handle.close()

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls._configure_logging()
        cls._start_server()

    @classmethod
    def tearDownClass(cls):
        cls._stop_server()
        super().tearDownClass()

    def setUp(self):
        super().setUp()
        proc = self.__class__.process
        if proc.poll() is not None:
            self._logger.warning(
                f"Server crashed (exit code {proc.returncode}); restarting"
            )
            self.__class__.restart_server()

    def send_request(self, method, path, headers=None, body=None, retry=True):
        """
        Send an HTTP/1.1 request to the server.
        Returns: (status, reason, headers dict, body bytes)
        Automatically retries once on connection errors.
        """
        ATTEMPTS = 3
        for attempt in range(ATTEMPTS):
            try:
                conn = http.client.HTTPConnection(self.HOST, self.PORT)
                conn.request(method, path, body=body, headers=headers or {})
                resp = conn.getresponse()
                resp_body = resp.read()
                resp_headers = dict(resp.getheaders())
                conn.close()
                return resp.status, resp.reason, resp_headers, resp_body

            except (ConnectionResetError, BrokenPipeError, socket.error) as e:
                self._logger.warning(
                    f"Connection error on attempt {attempt} ({e}); \
                                   {'retrying and restarting server' if attempt == 1 else 'giving up'}"
                )
                conn.close()
                if attempt != ATTEMPTS - 1:
                    self.__class__.restart_server()
                    continue
                else:
                    raise

        # Should not reach here
        raise RuntimeError("send_request failed unexpectedly")


class IntegrationTests(HTTPServerTestCase):
    SERVER_CMD = ["./bin/webserver", "./server_config.txt"]
    SERVER_CWD = "."  # optional cwd
    HOST = "localhost"  # server host (default 'localhost')
    PORT = 8081  # server port (int)
    STDERR_FILE = "integration_test_logs/server_stderr.log"
    TESTLOG_FILE = "integration_test_logs/server_test.log"
    STARTUP_DELAY = 2  # seconds to wait after starting server

    def test_common_data_dir(self):
        self._logger.info(f"common file path: {test_data_dir}")
        self.assertTrue(os.path.exists(test_data_dir))

    def test_echo(self):
        status, reason, headers, body = self.send_request(
            "GET", "/echo", body="hello world"
        )
        self.assertEqual(status, 200)

    def test_bad_prefix(self):
        status, reason, headers, body = self.send_request("GET", "/foobar")
        self.assertEqual(status, 404)

    def test_path_traversal_attack(self):
        status, reason, headers, body = self.send_request("GET", "/../pwned")
        self.assertIn(status, {403, 404})

    def test_text_file(self):
        status, reason, headers, body = self.send_request("GET", "/static/index.txt")
        self.assertEqual(status, 200)
        self.assertIn("Content-Length", headers)
        self.assertIn("Content-Type", headers)

    def test_zip_file(self):
        status, reason, headers, body = self.send_request("GET", "/static/index.zip")
        self.assertEqual(status, 200)
        self.assertIn("Content-Length", headers)
        self.assertIn("Content-Type", headers)

    def test_html_file(self):
        status, reason, headers, body = self.send_request("GET", "/static/index.html")
        self.assertEqual(status, 200)
        self.assertIn("Content-Length", headers)
        self.assertIn("Content-Type", headers)

    def test_full_echo_request(self):
        status, reason, headers, body = self.send_request(
            "GET",
            "/echo",
            headers={"Host": "localhost", "Cookie": "int-testing"},
        )

        self.assertEqual(status, 200)
        self.assertIn(b"Host: localhost", body)
        self.assertIn(b"Cookie: int-testing", body)

    def test_404_handler(self):
        self._logger.info("test_404_handler test")
        status, reason, headers, body = self.send_request(
            "GET",
            "/",
            headers={"Host": "localhost", "Cookie": "int-testing"},
        )

        self._logger.info("response body: %s", body)
        self.assertEqual(status, 404)
        self.assertIn("Content-Type", headers)
        self.assertIn(b"404 Not Found", body)

if __name__ == "__main__":
    unittest.main(verbosity=2)
