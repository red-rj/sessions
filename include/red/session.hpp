#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>

#include <range/v3/view/split.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/iterator/common_iterator.hpp>

#include "ranges.hpp"

namespace red::session {

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
        // using value_range = decltype(detail::environ_keyval(*this,false));
        // using key_range = value_range;

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
        // variable operator [] (std::string const& k) const { return variable(k); }
        // variable operator [] (char const* k) const { return variable(k); }

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

        /*value_range*/ auto values() const noexcept {
            auto functor = detail::keyval_fn{false};
            return functor(*this);
        }
        /*key_range*/ auto keys() const noexcept {
            auto functor = detail::keyval_fn{true};
            return functor(*this);
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


#if defined(SESSION_NOEXTENTIONS)
    void init_args(int argc, const char** argv);
#endif

} /* namespace red::session */

#endif /* RED_SESSION_HPP */