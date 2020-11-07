Some personal observations, suggestions, notes
---

## Paper
- `noexcept` requirements are a bit optimistic...
- forcing [case insensitive checks](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1275r0.html#design-synopsis-environment) for key lookup is a **bad** idea, what happens when 2+ entries are ambiguous? Just make it implementation defined.
