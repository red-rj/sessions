#ifndef IXM_SESSION_IMPL_HPP
#define IXM_SESSION_IMPL_HPP

#include <type_traits>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

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
        difference_type operator - (const charbuff_iterator& rhs) {
            return m_buff - rhs.m_buff;
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


    inline auto split_line(std::string_view line) {
        const size_t eqpos = line.find('='), sz = line.size();
        if (eqpos == std::string::npos) 
            return std::pair<std::string_view, std::string_view>{};

        return std::pair{line.substr(0, sz - eqpos), line.substr(eqpos + 1)};
    }

    template<typename CharT>
    class pathsep_iterator
    {
    public:
        using value_type = std::basic_string_view<CharT>;
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

        // pathsep_iterator& operator -- (int) {
        //     next_sep(false);
        //     return *this;
        // }

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
        void next_sep()
        {
            if (m_offset == std::string::npos) {
                m_view = {};
                return;
            }

            auto pos = m_var.find(Sep, m_offset);
            
            if (pos == std::string::npos) {
                m_view = m_var.substr(m_offset, pos);
                m_offset = pos;
                return;
            }
            
            m_view = m_var.substr(m_offset, pos-m_offset);
            m_offset = pos+1;
        }

        // void prev_sep() {
        //     ;
        // }
    
        value_type m_view, m_var;
        CharT Sep = 
#if defined(_WIN32)
        ';' ;
#else
        ':';
#endif // _WIN32
        
        
        size_t m_offset = 0;
    };

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
    using ci_string = std::basic_string<char, ci_char_traits>;

} // ixm::session::detail


#endif