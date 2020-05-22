#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>
#include <utility>

#include <range/v3/view/delimit.hpp>
#include <range/v3/view/split.hpp>

#include "ranges.hpp"

namespace red::session {

    class environment;

namespace detail {
    using splitpath_rng = ranges::split_view<ranges::views::all_t<std::string&>, ranges::single_view<char>>;
    using splitpath_iter = decltype(std::declval<splitpath_rng>().begin());
} // namespace detail


    class environment
    {

    public:
        class variable
        {
        public:
            using path_iterator = detail::splitpath_iter;
        
            operator std::string_view() const noexcept { return value(); }
            std::string_view key() const noexcept { return m_key; }
            std::string_view value() const noexcept;
            detail::splitpath_rng split () const;

            explicit variable(std::string_view key_) : m_key(key_) {}
            variable& operator=(std::string_view value);

        private:
            std::string& inner_val() const;
        
            std::string m_key;
        };

        using iterator = detail::environ_iterator;
        using value_type = variable;
        using size_type = size_t;
        // using value_range = decltype(detail::environ_keyval(*this,false));
        // using key_range = value_range;
        friend class variable;

        environment() noexcept;

        template <class T>
        using Is_Strview = std::enable_if_t<
            std::conjunction_v<
                std::is_convertible<const T&, std::string_view>, 
                std::negation<std::is_convertible<const T&, const char*>>
            >
        >;

        template <class T>
        using Is_Strview_Convertible = std::enable_if_t<std::is_convertible_v<const T&, std::string_view>>;


        template <class T, class = Is_Strview<T>>
        variable operator [] (T const& k) const { return variable(k); }
        variable operator [] (std::string_view k) const { return variable(k); }
        // variable operator [] (std::string const& k) const { return variable(k); }
        // variable operator [] (char const* k) const { return variable(k); }

        template <class K, class = Is_Strview_Convertible<K>>
        iterator find(K const& key) const noexcept { return find(std::string_view(key)); }
        iterator find(std::string_view k) const noexcept;

        bool contains(std::string_view key) const;

        iterator cbegin() const noexcept { return iterator(sys::envp()); }
        /*iterator*/ auto cend() const noexcept { return ranges::default_sentinel; } // TODO: why doesn't sentinel convert to iterator?
        auto begin() const noexcept { return cbegin(); }
        auto end() const noexcept { return cend(); }

        size_type size() const noexcept;
		[[nodiscard]] bool empty() const noexcept { return size() == 0; }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) { erase(std::string_view(key)); }
        void erase(std::string_view key);

        /*value_range*/ auto values() const noexcept {
            return detail::environ_keyval(*this, false);
        }
        /*key_range*/ auto keys() const noexcept {
            return detail::environ_keyval(*this, true);
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

    template<class Iter>
    std::string join_paths(Iter begin, Iter end);

    template<class Range>
    std::string join_paths(Range rng);

#if defined(SESSION_NOEXTENTIONS)
    void init_args(int argc, const char** argv);
#endif

} /* namespace red::session */

#endif /* RED_SESSION_HPP */