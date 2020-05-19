#ifndef RED_SESSION_IMPL_HPP
#define RED_SESSION_IMPL_HPP

#include <type_traits>
#include <iterator>
#include <string_view>


namespace red::session
{
namespace detail
{

    class pathsep_iterator
    {
    public:
        using value_type = std::basic_string_view<char>;
        using reference = value_type&;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using iterator_category = std::forward_iterator_tag;

        pathsep_iterator() = default;

        explicit pathsep_iterator(value_type str) : m_var(str) {
            next_sep();
        }

        pathsep_iterator& operator ++ () {
            next_sep();
            return *this;
        }
        pathsep_iterator operator ++ (int) {
            auto tmp = pathsep_iterator(*this);
            operator++();
            return tmp;
        }

        bool operator == (const pathsep_iterator& rhs) {
            return m_view == rhs.m_view;
        }
        bool operator != (const pathsep_iterator& rhs) {
            return !(*this == rhs);
        }

        reference operator * () {
            return m_view;
        }

    private:
        void next_sep() noexcept;
    
        value_type m_view, m_var;
        size_t m_offset = 0;
    };

} // detail

} // red::session

#endif