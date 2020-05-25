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


// cursor over a C array of pointers where the end is nullptr
template<typename T>
class c_ptrptr_cursor
{
    T** block = nullptr;
public:
    void next() noexcept { block++; }
    void prev() noexcept { block--; }
    void advance(ptrdiff_t n) noexcept { block += n; }
    T* read() const noexcept { return *block; };
    ptrdiff_t distance_to(c_ptrptr_cursor const &that) const noexcept {
        return that.block - block;
    }
    bool equal(c_ptrptr_cursor const& other) const noexcept {
        return block == other.block;
    }
    bool equal(ranges::default_sentinel_t) const noexcept {
        return *block == nullptr;
    }

    c_ptrptr_cursor()=default;
    c_ptrptr_cursor(T** ep) : block(ep) {
        assert(block != nullptr);
    }
};

class environ_cursor : c_ptrptr_cursor<sys::envchar>
{
    using base_t = c_ptrptr_cursor<sys::envchar>;

    std::string current;

public:
    std::string_view read() const { return current; }

    void next() {
        base_t::next();
        auto* native = base_t::read();
        current = sys::narrow(native);
    }
    void prev() {
        base_t::prev();
        auto* native = base_t::read();
        current = sys::narrow(native);
    }
    void advance(ptrdiff_t n) {
        base_t::advance(n);
        auto* native = base_t::read();
        current = sys::narrow(native);
    }

    // note: inheriting methods (using base_t::...) don't work

    bool equal(environ_cursor const& other) const noexcept { return base_t::equal(other); }
    bool equal(ranges::default_sentinel_t ds) const noexcept { return base_t::equal(ds); }
    ptrdiff_t distance_to(environ_cursor const &that) const noexcept { return base_t::distance_to(that); }
    
    environ_cursor() = default;
    environ_cursor(sys::env_t penv) : base_t(penv) {
        current = sys::narrow(*penv);
    }
};

using environ_iterator = ranges::basic_iterator<environ_cursor>;

} // namespace red::session::detail
