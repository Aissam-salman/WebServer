#!/usr/bin/env python3

import os
import sys


class Request:
    def __init__(
        self,
        method: str,
        content_length: int,
        content_type: str,
        path_translated: str,
        query_string: str,
        resp: str = "",
        body=None
    ) -> None:
        self.method = method
        self.content_length = content_length
        self.content_type = content_type
        self.path_translated = path_translated
        self.query_string = query_string
        self.resp = resp
        self.body = body

    def Get(self):
        print("GET")
        
            # std::string resp =
            #     "HTTP/1.1 200 OK\r\nContent-Length: 17\r\nContent-Type: "
            #     "text/plain\r\nConnection: close\r\n\r\nRESP FROM WEBSERV";
        # path = self.path_translated + "/index.html"
        print(os.pardir)
        # with open(path) as f:
        #     print(f.read())
        #script name already this script 
        # need handle path translated, try to read file, if not, method ?? in get, 
        # if  have query add query to file maybe ?? 

    def Post(self):
        print("POST")
        self.body = sys.stdin.read(self.content_length)
    def Delete(self):
        print("DELETE")

    def dispatch(self) -> None:
        match self.method:
            case "GET":
               self.Get()
            case "POST":
                self.Post()
            case "DELETE":
                self.Delete()
            case _:
                print("OTHER")


# ton script écrit les headers HTTP + une ligne vide + le body
# Content-Type: text/html\r\n
# \r\n
# <html>...ton contenu...</html>


def debug():
    print("-------------- [DEBUG]  ------------------")
    print("")
    print("Hello from CGI python")
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
    print(f"PATH_INFO={os.environ.get('PATH_INFO')}")
    print(f"PATH_TRANSLATED={os.environ.get('PATH_TRANSLATED')}")
    print(f"QUERY_STRING={os.environ.get('QUERY_STRING')}")
    print(f"CONTENT_TYPE={os.environ.get('CONTENT_TYPE')}")
    print("")
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    if (content_length > 0):
        print("body: ")
        body = sys.stdin.read(content_length)
        print(body)
    print("----------- END DEBUG -------------------")
    sys.stdout.flush()


def main() -> int:
    debug()
    rq = Request(
        method=os.environ.get("REQUEST_METHOD", ""),
        content_length=int(os.environ.get("CONTENT_LENGTH", 0)),
        content_type=os.environ.get("CONTENT_TYPE", ""),
        path_translated=os.environ.get("PATH_TRANSLATED", ""),
        query_string=os.environ.get("QUERY_STRING", ""),
    )
    rq.dispatch()
    return 0

main()
