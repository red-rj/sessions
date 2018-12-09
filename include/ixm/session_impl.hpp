#ifndef SESSION_IMPL_HPP
#define SESSION_IMPL_HPP

#include <type_traits>
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

        explicit charbuff_iterator(const char** buff = nullptr) : m_buff(buff)
        {}

        charbuff_iterator& operator ++ ()
        {
            m_buff++;
            return *this;
        }
        charbuff_iterator operator ++ (int) {
            auto tmp = charbuff_iterator(*this);
            operator++();
            return tmp;
        }

        charbuff_iterator& operator -- ()
        {
            m_buff--;
            return *this;
        }
        charbuff_iterator operator -- (int) {
            auto tmp = charbuff_iterator(*this);
            operator--();
            return tmp;
        }

        value_type operator [] (difference_type n) {
            return m_buff[n];
        }

        charbuff_iterator& operator += (difference_type n) {
            m_buff += n;
            return *this;
        }
        charbuff_iterator& operator -= (difference_type n) {
            m_buff -= n;
            return *this;
        }

        charbuff_iterator operator + (difference_type n) {
            auto tmp = charbuff_iterator(*this);
            return tmp += n;
        }

        charbuff_iterator operator - (difference_type n) {
            auto tmp = charbuff_iterator(*this);
            return tmp -= n;
        }

        bool operator == (const charbuff_iterator& rhs) const {
            return m_buff == rhs.m_buff;
        }
        bool operator != (const charbuff_iterator& rhs) const {
            return !(*this == rhs);
        }
        bool operator < (const charbuff_iterator& rhs) const {
            return rhs.m_buff - m_buff > 0;
        }
        bool operator > (const charbuff_iterator& rhs) const {
            return rhs.m_buff < m_buff;
        }
        bool operator >= (const charbuff_iterator& rhs) const {
            return !(*this < rhs);
        }
        bool operator <= (const charbuff_iterator& rhs) const {
            return !(*this > rhs);
        }

        reference operator * () {
            return *m_buff;
        }

    protected:
        auto buffer() const noexcept { return m_buff; }

    private:
        pointer m_buff = nullptr;
    };

    // charbuff_iterator operator + (ptrdiff_t n, charbuff_iterator it) {
    //     return it += n;
    // }


    class env_iterator : detail::charbuff_iterator
    {
    public:
        using value_type = std::pair<std::string_view, std::string_view>;
        using reference = value_type;

        explicit env_iterator(const char** buff = nullptr) 
        : charbuff_iterator(buff)
        {
            if (buff)
                m_current = split_line(*buff);
        }



        reference operator * () {
            return m_current;
        }

    private:
        value_type split_line(const char* str) {
            std::string_view line = str;
            const auto eqpos = line.find('=');

            auto retval = value_type(line, line);
            retval.first.remove_suffix(line.size() - eqpos);
            retval.second.remove_prefix(eqpos + 1);

            return retval;
        }

        value_type m_current;
    };
} // ixm::session::detail


#endif