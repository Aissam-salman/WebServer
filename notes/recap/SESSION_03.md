# Session 03 — 2026-06-18

## CGI

- HTTP requests for CGI have two parts separated by `\r\n\r\n`: **headers** and **body**
- The body (e.g. `username=alice&password=1234`) is passed to the CGI script via **stdin**
- Meta-variables are **environment variables** built by the server and passed to `execve` via `envp[]`
- They come from two sources:
  - **HTTP request**: `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`, `SERVER_PROTOCOL`, `HTTP_*` headers
  - **Server config**: `SERVER_NAME`, `SERVER_PORT`, `SERVER_SOFTWARE`, `GATEWAY_INTERFACE`, `DOCUMENT_ROOT`, `SCRIPT_FILENAME`
- `PATH_INFO` = extra path after the script name in the URL (e.g. `/cgi-bin/app.py/user/42` → `PATH_INFO=/user/42`)
- `PATH_TRANSLATED` = `DOCUMENT_ROOT` + `PATH_INFO`

## Lexer — ifstream as class member

- `std::ifstream` is **not copyable** → can't return it from a function, can't copy a `Lexer`
- In C++98, to prevent copying: declare copy constructor and assignment operator `private` without implementing them
- Initialize `_raw_conf_file` in the **member initializer list** (not the constructor body)
- Use `.c_str()` because `std::ifstream` takes `const char*` in C++98

```cpp
Lexer::Lexer(std::string name): _name(name), _raw_conf_file(name.c_str()) {
    if (name.size() < 5 || name.substr(name.size() - 5) != ".conf")
        throw std::runtime_error("Config file must end with .conf");
    if (!_raw_conf_file.is_open())
        throw std::runtime_error("Cannot open config file: " + name);
}
```

## Reading the file

- Use `std::getline` in a loop to read line by line
- `std::ifstream` has an internal read cursor — reading exhausts it
- To rewind: `clear()` first (resets `eofbit`/`failbit`), then `seekg(0)`

```cpp
_raw_conf_file.clear();
_raw_conf_file.seekg(0);
```

## Misc fixes

- `endofline` is a **stream manipulator** — use it like `std::endl`, no parentheses: `std::cout << value << endofline;`
- Functions defined in a header cause **duplicate symbol** linker errors — keep only the declaration in `.hpp`, move the definition to `.cpp`
- `std::runtime_error` message is retrieved with `e.what()` in a `catch (std::exception &e)` block
- A `const` method makes all members `const` — `std::getline` requires a non-const stream, so remove `const` from methods that read the file
