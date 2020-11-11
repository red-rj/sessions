# Red Sessions
Red Sessions is a library for accessing arguments and environment variables from anywhere in your program.

It's an implementation of the proposed P1275's Desert Sessions API - See link above for details on the paper it self.

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
- The `join_paths` function allows joining a series of `std::filesystem::path` into a `std::string`

Both `arguments` and `environment` are empty classes and can be freely constructed around.

Feel free to open an issue or contact me if you want to share some feedback. 😃

## How to use
### Arguments
It's as simple as creating an instance and using it like a container. 

```cpp
#include "red/sessions/session.hpp"
using namespace red::session;

arguments args;

for (const auto& a : args) {
    // reading each arg
}

// ...

auto it = args.begin();
*it = "whatever"; // Error, can't modify/add elements to `arguments`

// copy the arguments if you want to modify them
std::vector<std::string> myargs{ args.begin(), args.end() };
```

### Environment
```cpp
#include "red/sessions/session.hpp"
using namespace red::session;

environment env;

// get a value by assinging it to a string
std::string myvar_value = env["myvar"];

// or keep the environment::variable object it self and do operations on it latter.
environment::variable mypath = env["PATH"];
std::string_view key = mypath.key() // "PATH"

// set
env["myvar"] = "something clever";
mypath = "a;b;c"; // assigns to PATH

// checking if a variable exists
if (env.contains("myvar")) {
    // do stuff
}

// iterating the environment
for (auto envline : env)
{
    /* 'envline' has the format key=value
    
        PATH=a;b;c
        myvar=something clever
        HOME=C:\Users\FulanoDeTal
        ...
    */
}

// erasing a variable
env.erase("myvar");
assert(!env.contains("myvar"));

// ...
```
