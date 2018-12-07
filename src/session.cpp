#include "ixm/session.hpp"
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
    // env
    environment::variable::operator std::string_view() const noexcept
    {
        return m_value;
    }
    
    auto environment::variable::operator=(std::string_view str) -> variable&
    {
        // TODO: check for null termination
        m_value = str;
        impl::set_env_var(key().data(), str.data());
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
        auto value = search_env(str);
        return value.empty() ? variable{ str } : variable{str, value};
    }

    auto environment::operator[] (const char*str) const noexcept -> variable
    {
        return operator[](std::string_view{ str });
    }

    bool environment::contains(std::string_view thingy) const noexcept
    {
        return !search_env(thingy).empty();
    }

    auto environment::cbegin() const noexcept -> iterator
    {
        return iterator{ impl::envp() };
    }

    auto environment::cend() const noexcept -> iterator
    {
        return iterator{impl::envp(), size()};
    }

    std::string_view environment::search_env(std::string_view thingy) const noexcept
    {
        ci_string_view key{ thingy.data(), thingy.length() };
        auto** env = impl::envp();

        for (size_t i = 0; env[i]; i++)
        {
            ci_string_view current = env[i];
            const auto eqpos = current.find('=');

            auto ck = current;
            std::string_view cv = { current.data(), current.length() };

            ck.remove_suffix(current.size() - eqpos);

            if (ck == key)
            {
                cv.remove_prefix(eqpos + 1);
                return cv;
            }
        }

        return {};
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
        return static_cast<size_type>(argc() - 1);
    }

    arguments::iterator arguments::cbegin () const noexcept
    {
        return iterator{};
    }

    arguments::iterator arguments::cend () const noexcept
    {
        return iterator(size());
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