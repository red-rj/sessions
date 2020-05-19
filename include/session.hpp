#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>

#include <range/v3/core.hpp>
#include <range/v3/view/transform.hpp>

namespace red::session {

namespace detail
{
    // range over a C array of pointers where the end is nullptr
    template<class T>
    class c_ptrptr_range : public ranges::view_facade<c_ptrptr_range<T>>
    {
        friend ranges::range_access;
        T** envp = nullptr;

        void next() { envp++; }
        T* read() const noexcept { return envp[0]; };
        bool equal(ranges::default_sentinel_t) const {
            return *envp == nullptr;
        }
        bool equal(c_ptrptr_range const& other) const {
            return envp == other.envp;
        }

    public:
        c_ptrptr_range() = default;
        explicit c_ptrptr_range(T** ep) : envp(ep) {}
    };

    struct environ_views_fn
    {
        template<class Ch>
        auto operator() (c_ptrptr_range<Ch>&& rng, bool key) const {
            using namespace ranges;
            return views::transform(rng, [key](std::basic_string_view<Ch> line) {
                auto const eq = line.find('=');
                return key ? line.substr(0, eq) : line.substr(eq+1);
            });
        }

    } inline constexpr environ_views;

    // HACK
    
    
    
} // namespace detail


    class environment
    {
        using env_range_t = detail::c_ptrptr_range<char>;

        static env_range_t env_range() noexcept;
        static size_t envsize() noexcept;
    public:
        class variable
        {
        public:
            struct path_iterator;
        
            operator std::string_view() const noexcept { 
                return m_value;
            }
            std::string_view value() const noexcept { 
                return this->m_value;
            }
            std::string_view key() const noexcept { return m_key; }
            std::pair<path_iterator, path_iterator> split () const;

            explicit variable(std::string_view key_) : m_key(key_) {
                m_value = query();
            }
            variable& operator = (std::string_view value);

        private:
            std::string query();
        
            std::string m_key;
            std::string m_value;
        };

        using value_range = decltype(detail::environ_views(env_range(),false));
        using key_range = decltype(detail::environ_views(env_range(),true));
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
        iterator find(K const& key) const noexcept { return find(key); }
        iterator find(std::string_view k) const noexcept;

        bool contains(std::string_view key) const;

        iterator cbegin() const noexcept { return env_range().begin(); }
        iterator cend() const noexcept { return iterator(); }
        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept { return envsize(); }
		[[nodiscard]] bool empty() const noexcept { return envsize() == 0; }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) { erase(key); }
        void erase(std::string_view key);

        value_range values() const noexcept {
            return detail::environ_views(env_range(), false);
        }
        key_range keys() const noexcept {
            return detail::environ_views(env_range(), true);
        }
    };

    struct environment::variable::path_iterator
    {
        using value_type = std::basic_string_view<char>;
        using iterator_category = std::bidirectional_iterator_tag;

        explicit path_iterator(value_type sv = {}) : m_var(sv)
        {}

        auto& operator++ () {
            next();
            return *this;
        }
        auto operator++ (int) {
            auto tmp = path_iterator(*this);
            next();
            return tmp;
        }

        auto& operator-- () {
            prev();
            return *this;
        }
        auto operator-- (int) {
            auto tmp = path_iterator(*this);
            prev();
            return tmp;
        }

        bool constexpr operator== (path_iterator const& rhs) const noexcept {
            return m_current == rhs.m_current;
        }
        bool constexpr operator!= (path_iterator const& rhs) const noexcept {
            return !(*this == rhs);
        }

        value_type& operator* () { return m_current; }


    private:
        void next() noexcept;
        void prev() noexcept;
    
        value_type m_var, m_current;
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
    
#endif /* RED_SESSION_HPP */
