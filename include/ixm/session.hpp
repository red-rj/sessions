#ifndef IXM_SESSION_HPP
#define IXM_SESSION_HPP

#include <string>
#include <string_view>
#include <utility>

#include "session_impl.hpp"

namespace ixm::session {

    struct environment
    {
        class variable
        {
        public:
            operator std::string_view() const noexcept;
            variable& operator = (std::string_view);
            std::string_view key() const noexcept;
            // /* implementation-defined */ split () const;

            explicit variable(std::string_view key_) : m_key(key_) {}
        private:
            std::string m_key;
        };

        using iterator = detail::charbuff_iterator;
        //using value_range = /* implementation-defined */;
        //using key_range = /* implementation-defined */;
        using value_type = variable;
        using size_type = size_t;


        template <class T>
        using Is_valid_key_type = std::enable_if_t<
            std::conjunction_v<
                std::is_convertible<const T&, std::string_view>, 
                std::negation<std::is_convertible<const T&, const char*>
                >
            >
        >;

        template <class T, class = Is_valid_key_type<T>>
        variable operator [] (T const& k) const {
            return operator[](k);
        }

        variable operator [] (std::string const&) const noexcept;
        variable operator [] (std::string_view) const;
        variable operator [] (char const*) const noexcept;

        template <class K, class = Is_valid_key_type<K>>
        iterator find(K const&) const noexcept {

        }

        bool contains(std::string_view) const noexcept;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept;
        bool empty() const noexcept { return size() == 0; }

        //value_range values() const noexcept;
        //key_range keys() const noexcept;

        template <class K, class = Is_valid_key_type<K>>
        void erase(K const&) noexcept {

        }

    private:
        // std::string_view search_env(std::string_view) const noexcept;

        mutable size_type m_envsize = 0;
        mutable bool m_size_cache_valid = false;
    };

    struct arguments
    {
        using iterator = detail::charbuff_iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using value_type = std::string_view;
        using index_type = size_t;
        using size_type = size_t;

        value_type operator [] (index_type) const noexcept;
        value_type at(index_type) const;

        [[nodiscard]] bool empty() const noexcept;
        size_type size() const noexcept;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        reverse_iterator crbegin() const noexcept;
        reverse_iterator crend() const noexcept;

        reverse_iterator rbegin() const noexcept { return crbegin(); }
        reverse_iterator rend() const noexcept { return crend(); }

        [[nodiscard]] const char** argv() const noexcept;
        [[nodiscard]] int argc() const noexcept;
    };

} /* namespace ixm::session */

#endif /* IXM_SESSION_HPP */
