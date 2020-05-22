Some personal observations, suggestions, notes
---

## Paper
- the paper needs to define what happens if external APIs modify the environment (`putenv()`, `setenv()`)
    - in case implementations decide to cache the environment, like mine!
- `noexcept` requirements are a bit optimistic...
- should `environment::variable` have a `value()` accessor as well?
- should `environment::variable` store a copy of the value?
    - an instance of `environment::variable` would hold the value associated w/ that key _at that point in time_ and be safe from invalidation
    - if some other API changed that env. variable, the user can just query `environment[]` again and get the correct result
- should `environment` have a `sync()` method that synchronizes external changes (`putenv`, `setenv`)?
- forcing [case insensetive checks](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1275r0.html#design-synopsis-environment) for key lookup is a **bad** idea, what happends when 2+ entries are ambiguous? Just make it implementation defined.


environ_view plan
---
it'll be a range that iterates through the environment pointer, it shall cache the current item into a string, calling a conversion functions if necessary (`to_utf8(s)`), and return a string_view into that. It re-uses the same string for every item.

Something like:

```cpp
struct environ_view : // one of the mysterious magical ranges::view_* base classes
{
    friend ranges::range_access;

    sys::env_t penv; // ptr to system env (environ or _wenviron)
    std::string current; // the current item

    void next() {
        penv++;
        current = sys::narrow(*penv);
    }
    void prev() {
        penv--;
        current = sys::narrow(*penv);
    }

    std::string_view read() const { return current; }

    // ...
}
```