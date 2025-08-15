# Red Sessions
Red Sessions is a library for accessing arguments and environment variables from anywhere in your program.

It's an implementation of the proposed P1275's Desert Sessions API - See [this link](https://wg21.link/p1275r0) for the paper.

## Objective
_Red Session's_ objective is to provide a set of safe, easy to use utilities to interact with the system's environment and program arguments anywhere, at any time.
These utilities should be straight to the point, acting as wrappers around non-portable OS facilities, applying conversions when necessary to keep the API consistent.

## Overview
What's in the package?

- `arguments` an _immutable_ wrapper around the old `argv` pointer, behaves like a `std::vector<T> const`.
- `environment` an _associative container_-like class, it let's you get and set environment variables like getting and setting keys in a `std::map`, except `environment::operator[]` returns a `environment::variable` object, see below.
- `environment::variable` is a proxy object for interacting with a single environment variable.
    It holds the value of a environment variable _at the time it's constructed_, it's not effected by changes to the system's environment.
    - To update the value of a `environment::variable`, call `environment::operator[]` again.
    - `environment::variable::split()` function returns a range-like object that can be used to iterate through variables like `PATH` that use your system's `path_separator`.
- The `join_paths` function allows joining a series of `std::filesystem::path` into a `std::string` using your system's `path_separator`, or a character of your choice.

Both `arguments` and `environment` are empty classes and can be freely constructed around.

Feel free to open an issue or contact me if you want to share some feedback. 😃

## How to use
### Arguments
It's as simple as creating an instance and using it like a container. Linux/Mac/BSD users may need additional setup, see note beelow.

```cpp
#include "red/sessions/session.hpp"

red::session::arguments arguments;

for (const auto& a : arguments) {
    // reading each arg
}

// ...

auto it = arguments.begin();
*it = "whatever"; // Error, can't modify/add elements to `arguments`

// copy the arguments if you want to modify them
std::vector<std::string> myargs{ arguments.begin(), arguments.end() };
```

**Note:**  By Default on non-Windows platforms we use the `gnu::constructor` extension attribute to get the program arguments.
if you don't want to/can't use compiler extensions, define `SESSIONS_NOEXTENTIONS`.

`arguments` will fallback to `/proc/self/cmdline` if available, reading from it once when first constructed.
Otherwise calling `arguments::init` is required.


### Environment
```cpp
#include "red/sessions/session.hpp"

red::session::environment environment;

// get a value by assinging it to a string
std::string myvar_value = environment["myvar"];

// or keep the environment::variable object it self and do operations on it latter.
auto mypath = environment["PATH"];
std::string_view key = mypath.key(); // "PATH"
std::string_view value = mypath.value(); // value of "PATH" when `mypath` was created

// set
environment["myvar"] = "something clever";
mypath = "a;b;c"; // assigns to PATH

// checking if a variable exists
if (environment.contains("myvar")) {
    // do stuff
}

// iterating the environment
for (auto envline : environment)
{
    /* 'envline' has the format key=value
    
        PATH=a;b;c
        myvar=something clever
        HOME=C:\Users\FulanoDeTal
        ...
    */
}

// erasing a variable
environment.erase("myvar");

// ...
```

## Building
Requires CMake 3.10 or later and optionaly Catch2 for the tests.

```sh
cd sessions
mkdir build
cmake -S . -B build -G "Your prefered build system"
cd build
your_prefered_build_command
```
