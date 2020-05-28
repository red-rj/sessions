#pragma once
#include <string>
#include <string_view>

#include <range/v3/iterator/basic_iterator.hpp>
#include <range/v3/view/transform.hpp>

#include "sys_layer.hpp"

namespace red::session::detail {

using std::ptrdiff_t;

struct environ_keyval_fn
{
    template<typename Rng>
    auto operator() (Rng&& rng, bool key) const {
        using namespace ranges;
        return views::transform(rng, [key](std::string_view line) {
            auto const eq = line.find('=');
            auto value = key ? line.substr(0, eq) : line.substr(eq+1);
            return std::string(value);
        });
    }

} inline constexpr environ_keyval;

class environ_cursor
{
    sys::env_t envblock = nullptr; // native environ block
    sys::env_t mutable pos = nullptr; // last read pos.
    std::string mutable current; // last read data

public:
    std::string_view read() const {
        if (pos != envblock) {
            auto* native = *envblock;
            current = sys::narrow(native);
            pos = envblock;
        }

        return current;
    }

    void next() {
        envblock++;
    }
    void prev() {
        envblock--;
    }
    void advance(ptrdiff_t n) {
        envblock += n;
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
    environ_cursor(sys::env_t penv) : envblock(penv), pos(penv)
    {
        current = sys::narrow(*envblock);
    }
};

using environ_iterator = ranges::basic_iterator<environ_cursor>;

} // namespace red::session::detail
