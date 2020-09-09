#ifndef RED_SESSIONS_HPP
#define RED_SESSIONS_HPP

#include <type_traits>
#include <string_view>
#include <string>

#include <range/v3/view/split.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/iterator/common_iterator.hpp>
#include <range/v3/iterator/basic_iterator.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/facade.hpp>

#include "config.h"

namespace red::session {

// system layer (impl detail)
namespace sys {
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

    char const** argv() noexcept;
    int argc() noexcept;
    
    std::string narrow(envchar const* s);
} // namespace sys

// impl detail
namespace detail {

    // cursor over an array of pointers where the end is nullptr
    template<typename T>
    class ptr_array_cursor
    {
        T** block = nullptr;
    public:
        void next() noexcept { block++; }
        void prev() noexcept { block--; }
        // void advance(ptrdiff_t n) noexcept { block += n; }
        T* read() const noexcept { return *block; };
        std::ptrdiff_t distance_to(ptr_array_cursor const &that) const noexcept {
            return that.block - block;
        }
        bool equal(ptr_array_cursor const& other) const noexcept {
            return block == other.block;
        }
        bool equal(ranges::default_sentinel_t) const noexcept {
            return *block == nullptr;
        }

        ptr_array_cursor()=default;
        ptr_array_cursor(T** ep) : block(ep) {
            //assert(block != nullptr);
        }
    };

    struct narrowing_cursor : ptr_array_cursor<sys::envchar>
    {
        using base_cursor = ptr_array_cursor<sys::envchar>;

        narrowing_cursor() = default;
        narrowing_cursor(sys::env_t e) : base_cursor(e)
        {
            elem = *e;
            current = sys::narrow(elem);
        }

        using value_type = std::string;

        value_type& read() const
        {
            auto cur = base_cursor::read();
            if (cur != elem) {
                current = sys::narrow(cur);
                elem = cur;
            }

            return current;
        }

    private:
        mutable sys::envchar* elem = nullptr; // last read elem.
        mutable std::string current; // last read data
    };

    class environ_view : public ranges::view_facade<environ_view>
    {
        friend ranges::range_access;
        using cursor = narrowing_cursor;
        auto begin_cursor() const { return cursor(sys::envp()); }
    };

    using environ_iterator = ranges::iterator_t<environ_view>;
    using environ_common_iter = ranges::common_iterator<environ_iterator, ranges::default_sentinel_t>;

    static_assert(ranges::bidirectional_iterator<environ_iterator>, "environ_iterator isn't bidir");


    inline auto get_key_view(const environ_view& rng)
    {
        using namespace ranges;
        return views::transform(rng, [](std::string const& line){
            auto const eq = line.find('=');
            return line.substr(0, eq);
        });
    }

    inline auto get_value_view(const environ_view& rng)
    {
        using namespace ranges;
        return views::transform(rng, [](std::string const& line){
            auto const eq = line.find('=');
            return line.substr(eq+1);
        });
    }

} // namespace detail

    class environment
    {
        detail::environ_view m_rng;
    public:
        class variable
        {
        public:
            class splitpath_t;
            friend class environment;
        
            operator std::string() const;
            std::string_view key() const noexcept { return m_key; }
            splitpath_t split () const;

            variable& operator=(std::string_view value);

        private:
            explicit variable(std::string_view key_) : m_key(key_) {}

            std::string m_key;
        };

        // the separator char. used in the PATH variable
        static const char path_separator;

        using iterator = detail::environ_common_iter;
        using value_type = variable;
        using size_type = size_t;
        using value_range = decltype(get_value_view(detail::environ_view{}));
        using key_range = decltype(get_key_view(detail::environ_view{}));

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

        iterator begin() const noexcept { return m_rng.begin(); }
        iterator end() const noexcept { return m_rng.end(); }
        iterator cbegin() const noexcept { return begin(); }
        iterator cend() const noexcept { return end(); }

        size_type size() const noexcept;
		[[nodiscard]] bool empty() const noexcept { return size() == 0; }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) { do_erase(key); }


        value_range values() const noexcept {
            return get_value_view(m_rng);
        }
        key_range keys() const noexcept {
            return get_key_view(m_rng);
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
            rng = range_t(m_value, environment::path_separator);
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
        /* Initialize arguments's global storage.
           Users need to call this function *ONLY* if SESSIONS_NOEXTENTIONS is set.
        */
        static void init(int argc, const char** argv) noexcept;
#endif // SESSION_NOEXTENTIONS

    };


    CPP_template(class Rng)
        (requires ranges::range<Rng> && concepts::convertible_to<ranges::range_value_t<Rng>, std::string_view>)
    std::string join_paths(Rng&& rng, char sep = environment::path_separator) {
        using namespace ranges;

        std::string var = rng | views::join(sep) | to<std::string>();
        if (var.back() == sep)
            var.pop_back();

        return var;
    }

    CPP_template(class Iter)
        (requires concepts::convertible_to<ranges::iter_value_t<Iter>, std::string_view>)
    std::string join_paths(Iter begin, Iter end, char sep = environment::path_separator) {
        return join_paths(ranges::subrange(begin, end), sep);
    }

} /* namespace red::session */

#endif /* RED_SESSIONS_HPP */