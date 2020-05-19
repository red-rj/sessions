#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>
#include <vector>

#include <range/v3/view/transform.hpp>
#include <range/v3/view/facade.hpp>
#include <range/v3/iterator.hpp>

namespace red::session {

namespace detail
{
    class pathsep_iterator;

    template<class T>
    class env_range : public ranges::view_facade<env_range<T>, ranges::finite>
    {
        friend ranges::range_access;
        T** envp = nullptr;

        void next() { envp++; }
        T* read() const noexcept { return envp[0]; };
        bool equal(ranges::default_sentinel_t) const {
            return *envp == nullptr;
        }

    public:
        env_range() = default;
        explicit env_range(T** ep) : envp(ep) {}
    };



    auto get_key_range(char** envp) noexcept {
        using namespace ranges;
        auto rng = env_range<char>(envp);
        return rng | views::transform([](std::string_view line) {
            auto eq = line.find('=');
            return line.substr(0, eq);
        });
    }
    static auto get_value_range(char** envp) noexcept {
        using namespace ranges;
        auto envrng = env_range<char>(envp);
        return envrng | views::transform([](std::string_view line) {
            auto eq = line.find('=');
            return line.substr(eq+1);
        });
    }
} // namespace detail


    class environment
    {
        static char** envp() noexcept;
        static size_t envsize() noexcept;
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

        using value_range = decltype(detail::get_value_range(nullptr));
        using key_range = decltype(detail::get_key_range(nullptr));
        using iterator = ranges::basic_iterator<detail::env_range<char>>;
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
        variable operator [] (std::string const& k) const { return variable(k); }
        variable operator [] (std::string_view k) const { return variable(k); }
        variable operator [] (char const* k) const { return variable(k); }

        template <class K, class = Is_Strview_Convertible<K>>
        iterator find(K const& key) const noexcept { return find(key); }
        iterator find(std::string_view k) const noexcept;

        bool contains(std::string_view key) const;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept { return envsize(); }
		[[nodiscard]] bool empty() const noexcept { return envsize() == 0; }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) { erase(key); }
        void erase(std::string_view key);

        value_range values() const noexcept {
            return detail::get_value_range(envp());
        }
        key_range keys() const noexcept {
            return detail::get_key_range(envp());
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
