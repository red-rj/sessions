# Red Sessions
Red Sessions is a library for accessing arguments and environment variables from anywhere in your program.

It's an implementation of the proposed P1275's Desert Sessions API - See link above for details on the paper it self.

## Overview
What's in the package?

- `arguments` an _immutable_ wrapper around the old `argv` pointer, behaves like a `std::vector<T> const`.
- `environment` an _associative container_-like class, it let's you get and set environment variables like getting and setting keys in a `std::map`, but with a couple of differences:
    - `environment::operator[]` returns `environment::variable`, a proxy for interacting a single environment variable. You may assign it to a string to get the value or keep an instance of it and do operations on it later.
- The `join_paths` function allows joining a series of `std::filesystem::path` into a `std::string`

Both are empty classes and can be freely constructed around.

Tested on Windows 10 and Kubuntu 18.04

Feel free to open an issue or contact me if you want to share some feedback. :)

## How to use
### Arguments
It's as simple as creating an instance and using it like a container. 

```cpp
#include "red/session.hpp"
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
#include "red/session.hpp"
using namespace red::session;

environment env;

// get a value by assinging it to a string
std::string myvar_value = env["myvar"];

// or keep the environment::variable object it self and do operations on it latter.
environment::variable mypath = env["PATH"];
auto splt = mypath.split();
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
