#include "ixm/session.hpp"
#include "impl.hpp"
#include <cstdlib>
#include <algorithm>


namespace
{
    struct ci_char_traits : public std::char_traits<char> {
        static char to_upper(char ch) {
            return toupper((unsigned char)ch);
        }
        static bool eq(char c1, char c2) {
            return to_upper(c1) == to_upper(c2);
        }
        static bool lt(char c1, char c2) {
            return to_upper(c1) < to_upper(c2);
        }
        static int compare(const char* s1, const char* s2, size_t n) {
            while (n-- != 0) {
                if (to_upper(*s1) < to_upper(*s2)) return -1;
                if (to_upper(*s1) > to_upper(*s2)) return 1;
                ++s1; ++s2;
            }
            return 0;
        }
        static const char* find(const char* s, int n, char a) {
            auto const ua(to_upper(a));
            while (n-- != 0)
            {
                if (to_upper(*s) == ua)
                    return s;
                s++;
            }
            return nullptr;
        }
    };

    using ci_string_view = std::basic_string_view<char, ci_char_traits>;
}

namespace ixm::session 
{
    // variable
    environment::variable::operator std::string_view() const noexcept
    {
        auto val = impl::get_env_var(m_key.c_str());
        
        if (val) {
            return val;
        } else {
            return {};
        }
    }
    
    auto environment::variable::operator=(std::string_view value) -> variable&
    {
        std::string val { value };
        impl::set_env_var(m_key.c_str(), val.c_str());
        return *this;
    }

    std::string_view environment::variable::key() const noexcept
    {
        return m_key;
    }

    auto environment::operator[] (const std::string& str) const noexcept -> variable
    {
        return operator[](str.c_str());
    }

    auto environment::operator[] (std::string_view str) const -> variable
    {
        std::string val { str };
        return operator[](val.c_str());
    }

    auto environment::operator[] (const char*str) const noexcept -> variable
    {
        auto value = impl::get_env_var(str);
        // if value is null means a new key might be added through the variable
        m_size_cache_valid = value != nullptr;
        return variable{str};
    }

    bool environment::contains(std::string_view thingy) const noexcept
    {
        return impl::get_env_var(thingy.data()) != nullptr;
    }

    auto environment::cbegin() const noexcept -> iterator
    {
        return iterator{ impl::envp() };
    }

    auto environment::cend() const noexcept -> iterator
    {
        return iterator{impl::envp() + size()};
    }

    auto environment::size() const noexcept -> size_type
    {
        if (m_size_cache_valid) return m_envsize;

        m_envsize = 0;
        auto env = impl::envp();
        while (env[m_envsize]) m_envsize++;

        m_size_cache_valid = true;

        return m_envsize;
    }


    // args
    auto arguments::operator [] (arguments::index_type idx) const noexcept -> value_type
    {
        return impl::argv(idx);
    }

    arguments::value_type arguments::at(arguments::index_type idx) const
    {
        if (idx >= size()) {
            throw std::out_of_range("invalid arguments subscript");
        }

        return impl::argv(idx);
    }

    bool arguments::empty() const noexcept
    {
        return size() == 0;
    }
    
    arguments::size_type arguments::size() const noexcept
    {
        return static_cast<size_type>(argc());
    }

    arguments::iterator arguments::cbegin () const noexcept
    {
        return iterator{argv()};
    }

    arguments::iterator arguments::cend () const noexcept
    {
        return iterator{argv() + argc()};
    }


    arguments::reverse_iterator arguments::crbegin () const noexcept
    {
        return reverse_iterator{ cend() };
    }

    arguments::reverse_iterator arguments::crend () const noexcept
    {
        return reverse_iterator{ cbegin() };
    }


    const char** arguments::argv() const noexcept
    {
        return impl::argv();
    }

    int arguments::argc() const noexcept
    {
        return impl::argc();
    }
}