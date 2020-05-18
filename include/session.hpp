#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>

#include <range/v3/view/transform.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/delimit.hpp>
#include <range/v3/view/facade.hpp>

namespace red::session {

namespace detail
{
    class pathsep_iterator;

    template<class T>
    class c_ptrptr_range : public ranges::view_facade<c_ptrptr_range>
    {
    private:
        friend ranges::range_access;
        T** ptr = nullptr;
        T const* read() const { return *ptr; }
        bool equal(ranges::default_sentinel_t) const { return *ptr == nullptr; }
        void next() { ++ptr; }
    public:
        c_ptrptr_range() = default;
        explicit c_ptrptr_range(T** p) : ptr(p) {}
    };
    
} // namespace detail


    class environment
    {
        static char** env() noexcept;
        static size_t envsize() noexcept;
        // using range_fn = std::string_view(*)(std::string_view);
        // using env_range_t = decltype(detail::c_array((char**)nullptr));
    public:
        class variable
        {
        public:
            using path_iterator = detail::pathsep_iterator;
        
            operator std::string_view() const noexcept { return this->value(); }
            std::string_view value() const noexcept { return this->m_value; }
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

        using iterator = char const**;
        using value_type = variable;
        using size_type = size_t;
        // using value_range = env_range_t;
        // using key_range = env_range_t;
        friend class variable;

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

        variable operator [] (std::string const& k) const { return variable(k); }
        variable operator [] (std::string_view k) const { return variable(k); }
        variable operator [] (char const* k) const { return variable(k); }

        template <class K, class = Is_Strview_Convertible<K>>
        iterator find(K const& key) const noexcept {
            return find(key);
        }
        iterator find(std::string_view k) const noexcept;

        bool contains(std::string_view key) const;


        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept { return envsize(); }
		[[nodiscard]] bool empty() const noexcept { return envsize() == 0; }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) {
            erase(key);
        }
        void erase(std::string_view key);

        auto values() const noexcept {
            using namespace ranges;
            auto envrange = views::delimit(env(), nullptr);
            return envrange | views::transform([](std::string_view envline) {
                return envline.substr(envline.find('=')+1);
            });
        }
        auto keys() const noexcept {
            using namespace ranges;
            auto envrange = views::delimit(env(), nullptr);
            return envrange | views::transform([](std::string_view envline) {
                return envline.substr(0, envline.find('='));
            });
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

#if defined(SESSION_NOEXTENTIONS)
    void init_args(int argc, const char** argv);
#endif

} /* namespace red::session */
    
#endif /* RED_SESSION_HPP */
