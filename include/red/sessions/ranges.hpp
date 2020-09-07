#pragma once
#include <string>
#include <string_view>

#include <range/v3/iterator/basic_iterator.hpp>
#include <range/v3/view/transform.hpp>

#include "sys_layer.hpp"

namespace red::session {
    class environment;
}

namespace red::session::detail {

using std::ptrdiff_t;

class environ_cursor
{
    sys::env_t envblock = nullptr; // native environ block
    sys::env_t mutable pos = nullptr; // last read pos.
    std::string mutable current; // last read data

public:
    std::string& read() const {
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

using envline_delegate = std::string(*)(const std::string&);

struct get_envline_key_fn
{
    template<typename Rng>
    auto operator() (Rng const& rng) const {
        using namespace ranges;
        return views::transform(rng, +[](const std::string& line) {
            auto const eq = line.find('=');
            auto key = line.substr(0, eq);
            return key;
        });
    }
} inline constexpr get_envline_key;

struct get_envline_value_fn
{
    template<typename Rng>
    auto operator() (Rng const& rng) const {
        using namespace ranges;
        return views::transform(rng, +[](const std::string& line) {
            auto const eq = line.find('=');
            auto val = line.substr(eq+1);
            return val;
        });
    }
} inline constexpr get_envline_value;

class environment_base
{
public:
    using iterator = ranges::common_iterator<detail::environ_iterator, ranges::default_sentinel_t>;

    iterator begin() const noexcept
    {
        return environ_iterator(sys::envp());
    }

    iterator end() const noexcept
    {
        return ranges::default_sentinel;
    }
};

using envkeyrng = ranges::transform_view<environment_base, get_envline_key_fn>;
using envvaluerng = ranges::transform_view<environment_base, get_envline_value_fn>;

} // namespace red::session::detail
