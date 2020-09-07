#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>
#include <string>

#include <range/v3/view/split.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/iterator/common_iterator.hpp>
#include <range/v3/iterator/basic_iterator.hpp>
#include <range/v3/view/transform.hpp>

#include "config.h"

namespace red::session {

// system layer
namespace sys
{
#ifdef WIN32
    using envchar = wchar_t;
#else
    using envchar = char;
#endif

    using env_t = envchar**;

    env_t envp() noexcept;

    std::string getenv(std::string_view key);
    void setenv(std::string_view key, std::string_view value);
    void rmenv(std::string_view key);

    inline constexpr char path_sep =
#ifdef WIN32
    ';';
#else
    ':';
#endif

    char const** argv() noexcept;
    int argc() noexcept;
    
    std::string narrow(envchar const* s);
} // namespace sys

namespace detail
{
    using std::ptrdiff_t;

    struct environ_keyval_fn
    {
        template<typename Rng>
        auto operator() (Rng&& rng, bool key) const {
            using namespace ranges;
            return views::transform(rng, [key](std::string const& line) {
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
} // namespace detail

    class environment
    {
    public:
        class variable
        {
        public:
            class splitpath_t;
            friend class environment;
        
            operator std::string() const { return sys::getenv(m_key); }
            std::string_view key() const noexcept { return m_key; }
            splitpath_t split () const;

            variable& operator=(std::string_view value) {
                sys::setenv(m_key, value);
                return *this;
            }

        private:
            explicit variable(std::string_view key_) : m_key(key_) {}

            std::string m_key;
        };

        using iterator = ranges::common_iterator<detail::environ_iterator, ranges::default_sentinel_t>;
        using value_type = variable;
        using size_type = size_t;

        template <class T>
        using Is_Strview = std::enable_if_t<
            std::conjunction_v<
                std::is_convertible<const T&, std::string_view>, 
                std::negation<std::is_convertible<const T&, const char*>>
            >
        >;

        template <class T>
        using Is_Strview_Convertible = std::enable_if_t<std::is_convertible_v<const T&, std::string_view>>;


        environment() noexcept;

        template <class T, class = Is_Strview<T>>
        variable operator [] (T const& k) const { return variable(k); }
        variable operator [] (std::string_view k) const { return variable(k); }
        variable operator [] (std::string const& k) const { return variable(k); }
        variable operator [] (char const* k) const { return variable(k); }

        template <class K, class = Is_Strview_Convertible<K>>
        iterator find(K const& key) const noexcept { return do_find(key); }

        bool contains(std::string_view key) const;

        iterator cbegin() const noexcept { return detail::environ_iterator(sys::envp()); }
        iterator cend() const noexcept { return ranges::default_sentinel; }
        auto begin() const noexcept { return cbegin(); }
        auto end() const noexcept { return cend(); }

        size_type size() const noexcept;
		[[nodiscard]] bool empty() const noexcept { return size() == 0; }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) { do_erase(key); }

        /* TODO: declare value_range and key_range
            How the hell do I make these?

            'using value_range = decltype(detail::environ_keyval(environment(),bool))' doesn't work, 
            cause environment is an incomplete type.

            detail::environ_keyval() calls ranges::view::transform()
        */

        /*value_range*/ auto values() const noexcept {
            return detail::environ_keyval(*this, false);
        }
        /*key_range*/ auto keys() const noexcept {
            return detail::environ_keyval(*this, true);
        }

    private:
        void do_erase(std::string_view key);
        iterator do_find(std::string_view k) const;
    };


    class environment::variable::splitpath_t
    {
        using range_t = ranges::split_view<ranges::views::all_t<std::string_view>, ranges::single_view<char>>;
        std::string m_value;
        range_t rng;

    public:
        splitpath_t(std::string_view val) : m_value(val) {
            rng = range_t(m_value, sys::path_sep);
        }

        auto begin() {
            return rng.begin();
        }
        auto end() {
            return rng.end();
        }
    };


    class arguments
    {
    public:
        using iterator = char const* const*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using value_type = std::string_view;
        using index_type = size_t;
        using size_type = size_t;

        value_type operator [] (index_type) const noexcept;
        value_type at(index_type) const;

        [[nodiscard]] bool empty() const noexcept { return size() == 0; }
        size_type size() const noexcept;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        reverse_iterator crbegin() const noexcept { return reverse_iterator{ cend() }; }
        reverse_iterator crend() const noexcept { return reverse_iterator{ cbegin() }; }

        reverse_iterator rbegin() const noexcept { return crbegin(); }
        reverse_iterator rend() const noexcept { return crend(); }

        [[nodiscard]] const char** argv() const noexcept;
        [[nodiscard]] int argc() const noexcept;

#ifdef SESSION_NOEXTENTIONS
        static void init(int argc, const char** argv) noexcept;
#endif
    };


    CPP_template(class Rng)
        (requires ranges::range<Rng> && concepts::convertible_to<ranges::range_value_t<Rng>, std::string_view>)
    std::string join_paths(Rng&& rng, char sep = sys::path_sep) {
        using namespace ranges;

        std::string var = rng | views::join(sep) | to<std::string>();
        if (var.back() == sep)
            var.pop_back();

        return var;
    }

    CPP_template(class Iter)
        (requires concepts::convertible_to<ranges::iter_value_t<Iter>, std::string_view>)
    std::string join_paths(Iter begin, Iter end, char sep = sys::path_sep) {
        return join_paths(ranges::subrange(begin, end), sep);
    }

} /* namespace red::session */

#endif /* RED_SESSION_HPP */