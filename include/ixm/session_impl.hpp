#ifndef SESSION_IMPL_HPP
#define SESSION_IMPL_HPP

#include <iterator>

namespace ixm::session::detail
{
    class charbuff_iterator
    {
    public:
        using value_type = const char*;
        using reference = value_type&;
        using difference_type = ptrdiff_t;
        using pointer = value_type * ;
        using iterator_category = std::random_access_iterator_tag;

        explicit charbuff_iterator(const char** buff = nullptr, size_t index = 0) :
            m_idx(index), m_buff(buff)
        {
            if (m_buff)
                m_current = m_buff[m_idx];
        }

        charbuff_iterator& operator ++ ()
        {
            m_current = m_buff[++m_idx];
            return *this;
        }

        charbuff_iterator operator ++ (int) {
            auto tmp = charbuff_iterator(*this);
            operator++();
            return tmp;
        }

        bool operator == (const charbuff_iterator& rhs) const {
            return m_current == rhs.m_current;
        }

        bool operator != (const charbuff_iterator& rhs) const {
            return !(*this == rhs);
        }

        reference operator * () {
            return m_current;
        }

    private:
        size_t m_idx = 0;
        const char** m_buff = nullptr;
        const char* m_current = nullptr;
    };
} // ixm::session::detail


#endif