#pragma once
#include <string>
#include <string_view>
#include <optional>

#include <range/v3/iterator/basic_iterator.hpp>
#include <range/v3/view/transform.hpp>

#include "sys_layer.hpp"

namespace red::session::detail {

class environ_cursor
{
    sys::env_t envblock = nullptr; // native environ block
    bool mutable dirty;
    std::optional<std::string> mutable cache;

public:
    using value_type = std::string;

    std::string read() const {
        if (dirty) {
            auto* native = *envblock;
            cache.emplace(sys::narrow(native));
            dirty=false;
        }

        return *cache;
    }

    void next() { advance(+1); }
    void prev() { advance(-1); }
    void advance(ptrdiff_t n) {
        envblock += n;
        dirty=true;
    }

    bool equal(environ_cursor const& other) const noexcept {
        return envblock == other.envblock;
    }
    bool equal(ranges::default_sentinel_t ds) const noexcept {
        return *envblock == nullptr;
    }
    ptrdiff_t distance_to(environ_cursor const &that) const noexcept {
        return that.envblock - envblock;
    }
    
    environ_cursor() = default;
    environ_cursor(sys::env_t penv) : envblock(penv)
    {
        dirty = true;
    }
};

using environ_iterator = ranges::basic_iterator<environ_cursor>;

struct keyval_fn
{
    bool getkey;

    template<typename Rng>
    auto operator() (Rng&& rng) const {
        using namespace ranges;
        return views::transform(rng, [=](std::string const& line) {
            auto const eq = line.find('=');
            auto value = getkey ? line.substr(0, eq) : line.substr(eq+1);
            return value;
        });
    }
};


// ---
// cursor over a C array of pointers where the end is nullptr
template<typename T>
class arrayptr_cursor
{
    T** block = nullptr;
public:
    using contiguous = std::true_type;

    void next() noexcept { block++; }
    void prev() noexcept { block--; }
    void advance(ptrdiff_t n) noexcept { block += n; }
    T* read() const noexcept { return *block; };
    ptrdiff_t distance_to(arrayptr_cursor const &that) const noexcept {
        return that.block - block;
    }
    bool equal(arrayptr_cursor const& other) const noexcept {
        return block == other.block;
    }
    bool equal(ranges::default_sentinel_t) const noexcept {
        return *block == nullptr;
    }

    arrayptr_cursor()=default;
    arrayptr_cursor(T** ep) : block(ep) {
        assert(block != nullptr);
    }
};

class native_environ_view : public ranges::view_facade<native_environ_view>
{
    friend ranges::range_access;
    using cursor = arrayptr_cursor<sys::envchar>;

    cursor begin_cursor() const {
        return {sys::envp()};
    }

public:
    native_environ_view()=default;
};

} // namespace red::session::detail
