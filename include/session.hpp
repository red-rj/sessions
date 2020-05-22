#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>
#include <memory>

#include <range/v3/core.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/delimit.hpp>
#include <range/v3/experimental/view/shared.hpp>

#include "sys_layer.hpp"

#define RED_ITERATOR_TYPES_ALL(vt, difft, ref, ptr, itcat) \
    using value_type=vt; using difference_type=difft; using reference=ref; \
    using pointer=ptr; using iterator_category=itcat

#define RED_ITERATOR_TYPES(vt, difft, itcat) RED_ITERATOR_TYPES_ALL(vt, difft, vt&, vt*, itcat)

namespace red::session {

    class environment;

namespace detail {

    // range over a C array of pointers where the end is nullptr
    struct c_ptrptr_fn
    {
        template<typename T, std::size_t N>
        auto operator() (T* (&t)[N]) const -> ranges::subrange<T**>
        {
            return {&t[0], &t[N-1]};
        }

        template<typename T>
        auto operator() (T** ary) const
        {
            return ranges::views::delimit(ary, (T*)nullptr);
        }

    } inline constexpr c_ptrptr;

    
    struct environ_keyval_fn
    {
        template<typename Rng>
        auto operator() (Rng&& rng, bool key) const {
            using namespace ranges;
            return views::transform(rng, [key](std::string_view line) {
                auto const eq = line.find('=');
                return key ? line.substr(0, eq) : line.substr(eq+1);
            });
        }

    } inline constexpr environ_keyval;

    // views::common para begin e end?

    class environ_range : public ranges::view_facade<environ_range>
    {
        friend ranges::range_access;
        friend environment;
        
        std::string current;
        sys::env_t penv = nullptr;

        // cursor
        void next();
        void prev();
        void advance(ptrdiff_t n);
        std::string_view read() const { return current; }
        bool equal(ranges::default_sentinel_t) const {
            return *penv == nullptr;
        }
        bool equal(environ_range const& other) const {
            return penv == other.penv;
        }

    public:
        // ctors
        environ_range() = default;
        explicit environ_range(sys::env_t env);
    };


    // value/key range 
    // iterator

    
    
} // namespace detail


    class environment
    {
        using env_range_t = detail::environ_range;

        // static auto env_range()->env_range_t {
        //     using namespace detail;
        //     return environ_range(sys::envp());
        // }

        env_range_t env_range = detail::environ_range(sys::envp());
    public:
        class variable
        {
        public:
            struct path_iterator;
        
            operator std::string_view() const noexcept { return value(); }
            std::string_view key() const noexcept { return m_key; }
            std::string_view value() const noexcept;
            std::pair<path_iterator, path_iterator> split () const;

            explicit variable(std::string_view key_) : m_key(key_) {}
            variable& operator=(std::string_view value);

        private:
            std::string m_key;
        };

        using value_range = decltype(detail::environ_keyval(env_range,false));
        using key_range = decltype(detail::environ_keyval(env_range,true));
        using iterator = ranges::iterator_t<env_range_t>;
        using value_type = variable;
        using size_type = size_t;
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

        iterator cbegin() const noexcept { return env_range.begin(); }
        auto cend() const noexcept { return env_range.end(); } // TODO: why doesn't sentinel convert to iterator?
        iterator begin() const noexcept { return cbegin(); }
        auto end() const noexcept { return cend(); }

        size_type size() const noexcept;
		[[nodiscard]] bool empty() const noexcept { return size() == 0; }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) { erase(std::string_view(key)); }
        void erase(std::string_view key);

        /*value_range*/ auto values() const noexcept {
            return detail::environ_keyval(env_range, false);
        }
        /*key_range*/ auto keys() const noexcept {
            return detail::environ_keyval(env_range, true);
        }
    };

    struct environment::variable::path_iterator
    {
        RED_ITERATOR_TYPES(std::string_view, ptrdiff_t, std::forward_iterator_tag);

        explicit path_iterator(std::shared_ptr<std::string> sv) : m_var(sv) {
            next();
        }
        path_iterator()=default;

        auto& operator++ () {
            next();
            return *this;
        }
        auto operator++ (int) {
            auto tmp = path_iterator(*this);
            next();
            return tmp;
        }

        bool constexpr operator== (path_iterator const& rhs) const noexcept {
            return m_current == rhs.m_current;
        }
        bool constexpr operator!= (path_iterator const& rhs) const noexcept {
            return !(*this == rhs);
        }

        reference operator* () { return m_current; }

    private:
        void next() noexcept;
    
        std::string_view m_current;
        std::shared_ptr<std::string> m_var;
        size_t m_offset=0;
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
    
#undef RED_ITERATOR_TYPES_ALL
#undef RED_ITERATOR_TYPES
#endif /* RED_SESSION_HPP */