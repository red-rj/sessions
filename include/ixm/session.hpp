#ifndef IXM_SESSION_HPP
#define IXM_SESSION_HPP

#include <string>
#include <string_view>
#include <utility>

#include "impl.hpp"

namespace ixm::session {

    struct environment
    {
        class variable
        {
            friend environment;

        public:
            operator std::string_view() const noexcept;
            variable& operator = (std::string_view);
            std::string_view key() const noexcept;
            // /* implementation-defined */ split () const;

            bool operator == (const variable& rhs) {
                return key() == rhs.key() && m_value == rhs.m_value;
            }

            bool operator != (const variable& rhs) {
                return !(*this == rhs);
            }

        private:
            explicit variable(std::string_view key_, std::string_view value_ = {})
                : m_value(value_), m_key(key_)
            {}

            std::string_view m_value, m_key;
        };

        using iterator = impl::charbuff_iterator;
        //using value_range = void /* implementation-defined */;
        //using key_range = void /* implementation-defined */;
        using value_type = variable;
        using size_type = size_t;

        environment()
        {
            auto e = impl::envp();
            while (e[m_envsize]) m_envsize++;
        }

        //template <class T>
        //variable operator [] (T const&) const;

        variable operator [] (std::string const&) const noexcept;
        variable operator [] (std::string_view) const;
        variable operator [] (char const*) const noexcept;

        template <class K>
        iterator find(K const&) const noexcept;

        bool contains(std::string_view) const noexcept;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept { return m_envsize; }
        bool empty() const noexcept { return m_envsize == 0; }

        //value_range values() const noexcept;
        //key_range keys() const noexcept;

        template <class K>
        void erase(K const&) noexcept;

    private:
        std::string_view search_env(std::string_view) const noexcept;

        size_type m_envsize = 0;
    };

    struct arguments
    {
        class iterator : public impl::charbuff_iterator
        {
        public:
            explicit iterator(size_t index = 0) : charbuff_iterator(impl::argv(), index) {}
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using value_type = std::string_view;
        using index_type = size_t;
        using size_type = size_t;

        value_type operator [] (index_type) const noexcept;
        value_type at(index_type) const noexcept(false);

        [[nodiscard]] bool empty() const noexcept;
        size_type size() const noexcept;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept;
        iterator end() const noexcept;

        reverse_iterator crbegin() const noexcept;
        reverse_iterator crend() const noexcept;

        reverse_iterator rbegin() const noexcept { return crbegin(); }
        reverse_iterator rend() const noexcept { return crend(); }

        [[nodiscard]] const char** argv() const noexcept;
        [[nodiscard]] int argc() const noexcept;
    };

} /* namespace ixm::session */

#endif /* IXM_SESSION_HPP */
