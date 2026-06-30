#!/usr/bin/env python3

from enum import Enum
import os
import sys
import sqlite3
import json
import logging
from urllib.parse import parse_qs
from email import message_from_string


class ContentType(Enum):
    X_WWW_FORM_URLENCODED = "application/x-www-form-urlencoded"
    JSON = "application/json"
    MULTIPART_FORM_DATA = "multipart/form-data"
    TEXT_PLAIN = "text/plain"


class Request:
    def __init__(
        self,
        method: str = "",
        script_filename: str = "",
        path_info: str = "",
        content_length: int = 0,
        content_type: str = "",
        path_translated: str = "",
        query_string: str = "",
        resp: str = "",
        body: str = "",
        conn: sqlite3.Connection = None,
    ) -> None:
        self.method = method
        self.script_filename = script_filename
        self.path_info = path_info
        self.content_length = content_length
        self.content_type = content_type
        self.path_translated = path_translated
        self.query_string = query_string
        self.resp = resp
        self.body = body
        self.conn = conn

    def _err_server(self):
        body = json.dumps({"error": "Internal Server Error"})
        print("Status: 500 Internal Server Error\r")
        print(f"Content-Type: application/json\r")
        print(f"Content-Length: {len(body)}\r")
        print("\r")
        print(body, end="")


    def _not_found(self):
        print("Status: 404 Not found\r")
        print("\r")

    def _bad_request(self):
        print("Status: 400 Bad Request\r")
        print("\r")

    def _OK(self, text):
        print("Status: 200 OK\r")
        print(f"Content-Length: {len(text)}\r")
        print("Content-Type: application/json\r")
        print("\r")
        print(text, end="")

    def _created(self, new_id):
        body = json.dumps({"id": new_id, "message": "Article created"})
        print("Status: 201 Created\r")
        print(f"Content-Type: application/json\r")
        print(f"Content-Length: {len(body)}\r")
        print("\r")
        print(body, end="")

    def _no_content(self):
        print("Status: 204 No Content\r")
        print("\r")

    def Get(self):
        self.conn.row_factory = sqlite3.Row
        cursor = self.conn.cursor()
        if self.query_string != "" and self.path_info != "":
            # - GET /cgi-bin/serve.py/article/1?lang=fr → retourne l'article 1 en français
            res = self.query_string.split("=")
            if res[0] != "lang":
                self._not_found()
                return
            if res[1] != "fr" and res[1] != "en":
                self._not_found()
                return
            lang = res[1]
            res2 = self.path_info.split("/")
            id = int(res2[-1])
            query = "select * from articles where id = ? and lang = ?"
            try:
                cursor.execute(
                    query,
                    (
                        id,
                        lang,
                    ),
                )
            except:
                self._not_found()
                return
            rows = cursor.fetchall()
            data = [dict(row) for row in rows]
            if len(data) == 0:
                self._not_found()
                return
            json_str = json.dumps(data)
            self._OK(json_str)
        elif self.query_string != "":
            # - GET /cgi-bin/serve.py?lang=fr → retourne tous les articles filtrés en français
            res = self.query_string.split("=")
            if res[0] != "lang":
                self._not_found()
                return
            if res[1] != "fr" and res[1] != "en":
                self._not_found()
                return
            lang = res[1]
            query = "select * from articles where lang = ?"
            try:
                cursor.execute(query, (lang,))
            except:
                self._not_found()
                return
            rows = cursor.fetchall()
            data = [dict(row) for row in rows]
            if len(data) == 0:
                self._not_found()
                return
            json_str = json.dumps(data)
            self._OK(json_str)
        elif self.path_info != "":
            # - GET /cgi-bin/serve.py/article/1 → retourne l'article 1 en français
            res = self.path_info.split("/")
            try:
                id = int(res[-1])
                query = "select * from articles where id = ?"
                cursor.execute(query, (id,))
                rows = cursor.fetchall()
                data = [dict(row) for row in rows]
                if len(data) == 0:
                    self._not_found()
                    return
                json_str = json.dumps(data)
                self._OK(json_str)
            except:
                self._not_found()
                return
        else:
            #  GET /cgi-bin/serve.py → retourne tous les articles
            query = "SELECT * from articles;"
            cursor.execute(query)
            rows = cursor.fetchall()
            data = [dict(row) for row in rows]
            json_str = json.dumps(data)
            self._OK(json_str)
        self.conn.commit()

    def Post(self):
        # print("[DEBUG] = POST")
        # print(f"[DEBUG] Content-Type -> {self.content_type}")
        if not self.content_type:
            self._bad_request()
            return
        title = lang = content = ""
        if self.content_type.startswith(ContentType.MULTIPART_FORM_DATA.value):
            params = self.parse_multipart()
            title = params.get("title", "")
            lang = params.get("lang", "")
            content = params.get("content", "")
        elif self.content_type == ContentType.X_WWW_FORM_URLENCODED.value:
            params = parse_qs(self.body)
            flat = {k: v[0] for k, v in params.items()}
            title = flat.get("title", "")
            lang = flat.get("lang", "")
            content = flat.get("content", "")
        elif self.content_type == ContentType.JSON.value:
            try:
                data = json.loads(self.body)
            except json.JSONDecodeError:
                self._bad_request()
                return
            required_key = {"title", "lang", "content"}
            if not required_key.issubset(data):
                self._bad_request()
                return
            title = data["title"]
            lang = data["lang"]
            content = data["content"]
        else:
            self._bad_request()
            return
        if not all((title, lang, content)):
            self._bad_request()
            return
        cursor = self.conn.cursor()
        query = "INSERT INTO articles (title, lang, content) VALUES (?, ?, ?);"
        cursor.execute(query, (title, lang, content))
        self.conn.commit()
        new_id = cursor.lastrowid
        self._created(new_id=new_id)

    def Delete(self):
        cursor = self.conn.cursor()
        if self.path_info == "":
            self._bad_request()
            return
        info = self.path_info.split("/")[1:]
        id = info[1]
        if len(info) == 2 and info[0] in "article" and int(id) > 0:
            query = "DELETE FROM articles WHERE id = ?"
            cursor.execute(query, (id))
            self.conn.commit()
            self._no_content()
            return
        self._bad_request()

    def Put(self):
        if not self.content_type or not self.path_info:
            self._bad_request()
            return
        info = self.path_info.split("/")[1:]
        if len(info) != 2 or info[0] != "article":
            self._bad_request()
            return

        try:
            id = int(info[1])
            if id <= 0:
                raise ValueError
        except ValueError:
            self._bad_request()
            return
        title = lang = content = ""

        if self.content_type.startswith(ContentType.MULTIPART_FORM_DATA.value):
            params = self.parse_multipart()
            title = params.get("title", "")
            lang = params.get("lang", "")
            content = params.get("content", "")
        elif self.content_type == ContentType.X_WWW_FORM_URLENCODED.value:
            params = parse_qs(self.body)
            flat = {k: v[0] for k, v in params.items()}
            title = flat.get("title", "")
            lang = flat.get("lang", "")
            content = flat.get("content", "")
        elif self.content_type == ContentType.JSON.value:
            try:
                data = json.loads(self.body)
            except json.JSONDecodeError:
                self._bad_request()
                return

            required_key = {"title", "lang", "content"}
            if not required_key.issubset(data):
                self._bad_request()
                return
            title = data["title"]
            lang = data["lang"]
            content = data["content"]
        else:
            self._bad_request()
            return

        if not all((title, lang, content)):
            self._bad_request()
            return
        cursor = self.conn.cursor()
        query = "UPDATE articles SET title = ?, lang = ?, content = ? WHERE id = ?"
        cursor.execute(
            query,
            (
                title,
                lang,
                content,
                id,
            ),
        )
        self.conn.commit()
        if cursor.rowcount == 0:
            self._not_found()
        else:
            self._no_content()

    def dispatch(self) -> None:
        if self.content_length > 0:
            self.body = sys.stdin.read(self.content_length)

        match self.method:
            case "GET":
                self.Get()
            case "POST":
                self.Post()
            case "PUT":
                self.Put()
            case "DELETE":
                self.Delete()
            case _:
                self._bad_request()

    def parse_multipart(self):
        raw = f"Content-Type: {self.content_type}\r\n\r\n{self.body}"
        msg = message_from_string(raw)
        params = {}
        for param in msg.get_payload():
            name = param.get_param("name", header="Content-Disposition")
            value = param.get_payload()
            params[name] = value
        return params


def debug():

    print("-------------- [DEBUG]  ------------------")
    print("")
    print("Hello from CGI python")
    print(f"REQUEST_METHOD={os.environ.get('REQUEST_METHOD')}")
    print(f"CONTENT_LENGTH={os.environ.get('CONTENT_LENGTH')}")
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
    print(f"SCRIPT_NAME={os.environ.get('SCRIPT_NAME')}")
    print(f"SCRIPT_FILENAME={os.environ.get('SCRIPT_FILENAME')}")
    print("")
    print("----------- END DEBUG -------------------")
    sys.stdout.flush()


def init_db(conn):
    cursor = conn.cursor()
    cursor.execute("""
            CREATE TABLE IF NOT EXISTS articles (
                id      INTEGER PRIMARY KEY AUTOINCREMENT,
                title   TEXT NOT NULL,
                lang    TEXT NOT NULL,
                content TEXT NOT NULL
            )
        """)
    query = "SELECT COUNT(*) from articles;"
    cursor.execute(query)
    size = cursor.fetchone()
    if size == 0:
        cursor.executemany(
            "INSERT OR IGNORE INTO articles (id, title, lang, content) VALUES (?, ?, ?, ?)",
            [
                (1, "Hello World", "en", "My first article in English."),
                (2, "Bonjour le monde", "fr", "Mon premier article en francais."),
                (
                    3,
                    "Introduction to C",
                    "en",
                    "Pointers, memory, and undefined behavior.",
                ),
                (4, "Les sockets POSIX", "fr", "Comment creer un serveur TCP en C."),
                (5, "CGI Explained", "en", "How a web server talks to a script."),
            ],
        )
    conn.commit()


def handle_exception(exc_type, exc_value, exc_traceback):
    logging.error("Exception non gérée", exc_info=(exc_type, exc_value, exc_traceback))


def main() -> int:
    logging.basicConfig(
        filename="cgi_error.log",
        level=logging.ERROR,
        format="%(asctime)s - %(levelname)s - %(message)s",
    )

    sys.stderr = open("www/cgi-bin/cgi_errors.log", "a")

    sys.excepthook = handle_exception
    conn = sqlite3.connect("www/cgi-bin/db.sql")
    init_db(conn)

    # debug()
    rq = Request(
        method=os.environ.get("REQUEST_METHOD", ""),
        script_filename=os.environ.get("SCRIPT_FILENAME", "www/cgi-bin/serve.py"),
        path_info=os.environ.get("PATH_INFO", ""),
        content_length=int(os.environ.get("CONTENT_LENGTH", 0)),
        content_type=os.environ.get("CONTENT_TYPE", ""),
        path_translated=os.environ.get("PATH_TRANSLATED", ""),
        query_string=os.environ.get("QUERY_STRING", ""),
        conn=conn,
    )
    try: 
        rq.dispatch()
    except:
        logging.error("Error: dispatch: %s", traceback.format_exc())
        pass
    conn.close()
    return 0


main()
