# Code Review — Session 03 (2026-06-18)

Files reviewed: `server/config/Lexer.hpp`, `server/config/Lexer.cpp`,
`server/config/configutils.hpp`, `server/config/configutils.cpp`,
`utils/utils.hpp`, `utils/utils.cpp`

---

## Critical Issues

### 1. Copy constructor and assignment operator are broken (`Lexer.cpp:24–35`)

`std::ifstream` is not copyable. The copy constructor does `*this = src`, which calls
`operator=`, which does nothing useful — meaning a copied `Lexer` has an unopen stream.
This is silent undefined behaviour.

The assignment operator logic is also **inverted**:
```cpp
// BUG: this returns early when objects ARE different, not when they're the same
if (this != &other)
    return (*this);
```
It should be `if (this == &other)` to guard against self-assignment, but even then
you can't copy the stream.

**Fix:** Since `Lexer` cannot be meaningfully copied, declare both `private` and leave
them unimplemented. Any accidental copy will then be a compile-time error, not silent
broken state.

```cpp
// In Lexer.hpp private section:
Lexer(const Lexer &src);
Lexer& operator=(const Lexer &other);
// No implementation in .cpp
```

---

### 2. `initTokensVector` doesn't populate `_raw_tokens_vector` (`Lexer.cpp:43–48`)

The method reads lines and calls `display(line)` — it prints them but never pushes
anything into `_raw_tokens_vector`. After calling it, the vector stays empty and
`printTokens` prints nothing.

```cpp
// Current (wrong):
while(std::getline(_raw_conf_file, line))
    display(line);

// Should be:
while(std::getline(_raw_conf_file, line))
    _raw_tokens_vector.push_back(line);
```

---

### 3. `configutils.hpp` has no include guard

The header has no `#ifndef`/`#define`/`#endif` guard. Including it twice in any
translation unit causes duplicate declarations.

```cpp
// Add at the top:
#ifndef CONFIGUTILS_HPP
# define CONFIGUTILS_HPP

// ... existing content ...

#endif
```

---

## Improvements

### 4. `static const std::string` arrays defined in a header (`Lexer.hpp:11–23`)

`GLOBAL_KEY`, `SERVER_KEY`, `LOCATION_KEY` are defined (not just declared) in the header
with `static`. In C++98, `static` here gives each translation unit that includes the
header its own separate copy of the arrays — wasted memory and repeated construction.

Move the definitions to a `.cpp` file (e.g. `configutils.cpp`) and declare them `extern`
in the header:

```cpp
// Lexer.hpp — declaration only:
extern const std::string GLOBAL_KEY[];
extern const size_t      GLOBAL_KEY_SIZE;

// configutils.cpp — one definition:
const std::string GLOBAL_KEY[] = { "server" };
const size_t      GLOBAL_KEY_SIZE = 1;
```

---

### 5. Double `"\n"` in `printTokens` (`Lexer.cpp:54`)

```cpp
std::cout << "TOKEN NB [" << i << "] = " << _raw_tokens_vector[i].c_str() << "\n" << endofline;
```

`"\n"` followed by `endofline` (which calls `std::endl`) outputs **two newlines**.
Remove one:

```cpp
std::cout << "TOKEN NB [" << i << "] = " << _raw_tokens_vector[i] << endofline;
```

Also: `.c_str()` is unnecessary when printing a `std::string` — `operator<<` handles it.

---

### 6. Duplicated `.conf` check (`Lexer.cpp:18` and `configutils.cpp:7–9`)

`checkConfFilepath` in `configutils.cpp` does the same `.conf` check that the Lexer
constructor already does. Pick one place — the Lexer constructor is already the right
place since it owns the file opening. Remove or never call `checkConfFilepath` for the
extension check.

---

### 7. `initErrorPages` is defined but inaccessible (`configutils.cpp:13–18`)

The function is defined in `configutils.cpp` but not declared in `configutils.hpp`.
No other file can call it. Either add the declaration to the header, or delete the
function until it's actually needed.

---

### 8. Default constructor leaves the Lexer in an unusable state (`Lexer.cpp:12–14`)

A default-constructed `Lexer` has an empty path and a closed stream. Calling any method
on it (`initTokensVector`, `printRawConfFile`) will silently do nothing. If there's no
real use case for a default `Lexer`, remove the default constructor entirely — forcing
callers to always provide a file path.

---

## Minor / Style

### 9. Duplicate `#include <string>` in `Lexer.hpp` (lines 4 and 6)

Remove one.

### 10. Commented-out `isValidKey` definition still in `Lexer.hpp` (lines 25–31`)

Dead code. Delete it — the real definition is in `utils.cpp`, the declaration is in
`utils.hpp`. The comment block adds noise.

### 11. `_raw_conf_file.clear()` + `seekg(0)` should stay together

In `printRawConfFile` this is correct. Just make sure they are always called together
whenever you rewind — `clear()` alone doesn't move the cursor, `seekg(0)` alone silently
fails if `eofbit` is set.

---

## What's Good

- Stream manipulator `endofline` is correctly declared in `.hpp` and defined once in
  `.cpp` — no duplicate symbol.
- `_raw_conf_file` correctly initialized in the member initializer list with `.c_str()`.
- File extension check and `is_open()` check in the constructor before any reads.
- `isValidKey` moved out of the header into `utils.cpp` — correct fix.
- `printTokens` correctly iterates the vector (not the stream) and is `const`.
