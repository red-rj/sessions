#include "ixm/session.hpp"
#include "impl.hpp"
#include <cstdlib>
#include <algorithm>


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

    void environment::internal_erase(const char* k) noexcept
    {
        impl::rm_env_var(k);
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