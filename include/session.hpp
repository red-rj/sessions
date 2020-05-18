#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include <type_traits>
#include <string_view>

#include "range/v3/view/transform.hpp"

namespace red::session {

namespace detail
{
    class pathsep_iterator;
} // namespace detail


    class environment
    {
    public:
        class variable
        {
        public:
            using path_iterator = detail::pathsep_iterator;
        
            operator std::string_view() const { return this->value(); }
            variable& operator = (std::string_view value);
            std::string_view key() const noexcept { return m_key; }
            std::string_view value() const;
            std::pair<path_iterator, path_iterator> split () const;

            explicit variable(std::string_view key_) : m_key(key_) {}
        private:
            std::string m_key;
        };

        using iterator = std::string const**;
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


        template <class T, class = Is_Strview<T>>
        variable operator [] (T const& k) const { return variable(k); }

        variable operator [] (std::string const& k) const { return variable(k); }
        variable operator [] (std::string_view k) const { return variable(k); }
        variable operator [] (char const* k) const { return variable(k); }

        template <class K, class = Is_Strview_Convertible<K>>
        iterator find(K const& key) const noexcept {
            return cache.find(key);
        }

        bool contains(std::string_view key) const {
            return cache.contains(key);
        }


        iterator cbegin() const noexcept { return cache.myenv.cbegin(); }
        iterator cend() const noexcept { return cache.myenv.cend(); }

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept { return cache.myenv.size(); }
		[[nodiscard]] bool empty() const noexcept { return cache.myenv.empty(); }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) {
            cache.rmvar(key);
        }

        auto values() const noexcept {
            using namespace ranges;
            return cache.myenv | views::transform([](std::string_view envline) {
                return envline.substr(envline.find('=')+1);
            });
        }
        auto keys() const noexcept {
            using namespace ranges;
            return cache.myenv | views::transform([](std::string_view envline) {
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
