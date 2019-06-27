# Red Sessions
Red Sessions is an implementation of the proposed P1275's Desert Sessions API - See link above for details on the paper it self.

## Features:
- Access program arguments and environment variables anywhere in a safe, easy way.
- UTF8 safe storage

## Overview
What's in the package?

- `arguments` is an _immutable_ wrapper around the old `argv` pointer, it's elements are the same for the lifetime of the application, behaves like a `std::vector<T> const`.
- `environment` is and an _associative conteiner_ like class, it let's you get and set environment variables like getting and setting keys in a `std::map`!

Both are empty classes and can be freely constructed around.

Tested on Windows 10 and Kubuntu 18.04

Feel free to open an issue or contact me if you want to share some feedback. :)

## How to use
### Arguments
It's as simple as creating an instance and using it like a conteiner.

```cpp
#include "red/session.hpp"
using namespace red::session;

arguments args;

for (const auto& a : args) {
    // reading each arg
}

// ...

auto it = args.begin();
// *it = "whatever"; // Error, can't modify/add elements to `arguments`

// copy the arguments if you need to modify them
std::vector<std::string> myargs{ args.begin(), args.end() };
```

### Environment
```cpp
#include "red/session.hpp"
using namespace red::session;

environment env;

// get
std::string_view myvar_value = env["myvar"];

// set
env["myvar"] = "...";

// checking if a variable exists
if (env.contains("myvar")) {
    // do stuff
}

// ...
```

## ToDo
- Implement envrionment ranges
- enforce utf-8 storage on non windows platforms