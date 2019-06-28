#ifndef RED_SESSION_HPP
#define RED_SESSION_HPP

#include "session_impl.hpp"
#include "range/v3/core.hpp"
#include "range/v3/view.hpp"

namespace red::session {

    class environment
    {
        struct line_elem_fn;
        
    public:
        class variable
        {
        public:
            using path_iterator = detail::pathsep_iterator;
        
            operator std::string_view() const { return this->value(); }
            variable& operator = (std::string_view value) {
                cache.setvar(m_key, std::string{value});
                return *this;
            }
            std::string_view key() const noexcept { return m_key; }
            std::string_view value() const { return cache.getvar(m_key); }
            std::pair<path_iterator, path_iterator> split () const {
                std::string_view value = *this;
                return { path_iterator{value}, path_iterator{} };
            }

            explicit variable(std::string_view key_) : m_key(key_) {}
        private:
            std::string m_key;
        };

        using value_range = ranges::transform_view<ranges::ref_view<detail::environ_cache::vector_t>, line_elem_fn>;
        using key_range = value_range;

        using iterator = detail::environ_cache::const_iterator;
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
            return cache.find(std::string{key});
        }

        bool contains(std::string_view key) const {
            return cache.contains(std::string{key});
        }


        iterator cbegin() const noexcept { return cache.myenv.cbegin(); }
        iterator cend() const noexcept { return cache.myenv.cend(); }

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept { return cache.myenv.size(); }
		[[nodiscard]] bool empty() const noexcept { return cache.myenv.empty(); }

        template <class K, class = Is_Strview_Convertible<K>>
        void erase(K const& key) {
            cache.rmvar(std::string{key});
        }

        value_range values() const noexcept {
            return ranges::transform_view(cache.myenv, line_elem_fn{false});
        }
        key_range keys() const noexcept {
            return ranges::transform_view(cache.myenv, line_elem_fn{true});
        }


    private:
        static detail::environ_cache cache;

        struct line_elem_fn {
            bool getkey;

            std::string_view operator()(std::string_view line) const noexcept {
                auto eq = line.find('=');
                return getkey ? line.substr(0, eq) : line.substr(eq);
            }
        };
    };



    class arguments
    {
    public:
        using iterator = detail::ptrarray_iterator<char const* const>;
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

} /* namespace red::session */
    
#endif /* RED_SESSION_HPP */
