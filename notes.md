Some personal suggestions / observations for the paper

- the paper needs to define what happens if external APIs modify the environment (`putenv()`, `setenv()`)
    - in case implementations decide to cache the environment, like mine!
- `noexcept` requirements are a bit optimistic...
- should `environment::variable` have a `value()` accesser as well?
- should `environment::variable` store a copy of the value?
    - an instance of `environment::variable` would hold the value associated w/ that key _at that point in time_ and be safe from invalidation
    - if some other API changed that env. variable, the user can just query `environment[]` again and get the correct result
- should `environment` have a `sync()` method that synchronizes external changes (`putenv`, `setenv`)?
