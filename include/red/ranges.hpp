#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <map>

#include <range/v3/iterator/basic_iterator.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/adaptor.hpp>

#include "sys_layer.hpp"

namespace red::session::detail {

struct environ_keyval_fn
{
    template<typename Rng>
    auto operator() (Rng&& rng, bool key) const {
        using namespace ranges;
        // using referance = range_reference_t<Rng>;
        return views::transform(rng, [=](std::string& line) {
            auto const eq = line.find('=');
            auto value = key ? line.substr(0, eq) : line.substr(eq+1);
            return value;
        });
    }

} inline constexpr environ_keyval;

class environ_cursor
{
    sys::env_t envblock = nullptr; // native environ block
    sys::env_t mutable pos = nullptr; // last read pos.
    std::string mutable current; // last read data

public:
    using value_type = std::string;
    // using single_pass = std::true_type;

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

struct keyval_fn
{
    bool getkey;

    template<typename Rng>
    auto operator() (Rng&& rng) const {
        using namespace ranges;
        return views::transform(rng, [=](iter_reference_t<Rng> line) {
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

class environ_view : public ranges::view_facade<environ_view>
{
    friend ranges::range_access;
    using cache_t = std::optional<std::string>;
    using native_iterator = ranges::iterator_t<native_environ_view>;

    native_environ_view native_view;
    cache_t cache;
    bool dirty;

    void update_(std::string&& val)
    {
        if (!cache)
            cache.emplace(val);
        else
            *cache = val;
    }

    struct cursor
    {
    private:
        environ_view *parent;
        native_iterator m_it;

    public:
        using value_type = std::string;

        cursor()=default;
        cursor(environ_view* r) 
        : parent(r), m_it(r->native_view.begin())
        {}

        void next() noexcept {
            m_it++;
            parent->dirty = true;
        }
        void prev() noexcept { 
            m_it--;
            parent->dirty = true;
        }
        std::string& read() const noexcept {
            if (parent->dirty) {
                parent->update_(sys::narrow(*m_it));
                parent->dirty = false;
            }
            return *parent->cache;
        };
        bool equal(cursor const& other) const noexcept {
            return m_it == other.m_it;
        }
        bool equal(ranges::default_sentinel_t) const noexcept {
            return m_it == parent->end();
        }
    };

    cursor begin_cursor()
    {
        dirty = true;
        return cursor{this};
    }

public:
    environ_view()=default;
};

using envview_iter = ranges::iterator_t<environ_view>;

} // namespace red::session::detail
