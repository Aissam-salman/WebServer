#!/usr/bin/env python3

import os 
import sys

print("Hello from CGI python")
print("")
print(f"REQUEST_METHOD={os.environ.get('REQUEST_METHOD')}")
print(f"CONTENT_LENGTH={os.environ.get('CONTENT_LENGTH')}")
print(f"REQUEST_METHOD={os.environ.get('REQUEST_METHOD')}")
print(f"SERVER_PROTOCOL={os.environ.get('SERVER_PROTOCOL')}")
print(f"SERVER_NAME={os.environ.get('SERVER_NAME')}")
print(f"SERVER_PORT={os.environ.get('SERVER_PORT')}")
print(f"GATEWAY_INTERFACE={os.environ.get('GATEWAY_INTERFACE')}")
print(f"SERVER_SOFTWARE={os.environ.get('SERVER_SOFTWARE')}")
print(f"REMOTE_ADDR={os.environ.get('REMOTE_ADDR')}")
print(f"REMOTE_PORT={os.environ.get('REMOTE_PORT')}")
print(f"SCRIPT_NAME={os.environ.get('SCRIPT_NAME')}")
print(f"SCRIPT_FILENAME={os.environ.get('SCRIPT_FILENAME')}")
print(f"PATH_INFO={os.environ.get('PATH_INFO')}")
print(f"PATH_TRANSLATED={os.environ.get('PATH_TRANSLATED')}")
print(f"QUERY_STRING={os.environ.get('QUERY_STRING')}")
print(f"CONTENT_TYPE={os.environ.get('CONTENT_TYPE')}")
print("")

print("body: ")
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length)
print(body)
sys.stdout.flush()

