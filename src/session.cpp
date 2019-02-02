#include "ixm/session.hpp"
#include "impl.hpp"


namespace ixm::session 
{
    // env::variable
    environment::variable::operator std::string_view() const
    {
        auto* val = impl::get_env_var(m_key.c_str());
        
        if (val) return val; else return {};
    }
    
    auto environment::variable::operator=(std::string_view value) -> variable&
    {
        std::string val { value };
        impl::set_env_var(m_key.c_str(), val.c_str());
        return *this;
    }

    auto environment::variable::split() const -> std::pair<path_iterator, path_iterator>
    {
        std::string_view value = *this;
        return { path_iterator{value}, path_iterator{} };
    }


    // env
    auto environment::operator[] (const std::string& str) const -> variable
    {
        return variable{ str };
    }

    auto environment::operator[] (std::string_view str) const -> variable
    {
        return variable{ str };
    }

    auto environment::operator[] (const char* str) const -> variable
    {
        return variable{ str };
    }

    bool environment::contains(std::string_view key) const
    {
        auto thingy = std::string(key);
        return impl::env_find(thingy.c_str()) != -1;
    }

    auto environment::cbegin() const noexcept -> iterator
    {
        return iterator{ m_envp() };
    }

    auto environment::cend() const noexcept -> iterator
    {
        return iterator{ m_envp() + size()};
    }

    auto environment::size() const noexcept -> size_type
    {
        return impl::env_size();
    }

    void environment::internal_erase(const char* k)
    {
        impl::rm_env_var(k);
    }

    bool environment::internal_find(const char* key, int& offset) const
    {
        offset = impl::env_find(key);
        return offset != -1;
    }

    char const** environment::m_envp() const noexcept { return impl::envp(); }



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

    const char** arguments::argv() const noexcept
    {
        return impl::argv();
    }

    int arguments::argc() const noexcept
    {
        return impl::argc();
    }


    // detail
    void detail::pathsep_iterator::next_sep() noexcept
    {
        if (m_offset == std::string::npos) {
            m_view = {};
            return;
        }

        auto pos = m_var.find(impl::path_sep, m_offset);

        if (pos == std::string::npos) {
            m_view = m_var.substr(m_offset, pos);
            m_offset = pos;
            return;
        }

        m_view = m_var.substr(m_offset, pos - m_offset);
        m_offset = pos + 1;
    }
}