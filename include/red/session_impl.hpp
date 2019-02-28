#ifndef RED_SESSION_IMPL_HPP
#define RED_SESSION_IMPL_HPP

#include <type_traits>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <mutex>


namespace red::session
{
	class environment;

namespace detail
{
	template<typename T>
    class ptrarray_iterator
    {
        static_assert(std::is_pointer_v<T>, "T must be a ptr type");
    public:
        using value_type = T;
        using reference = value_type&;
        using pointer = value_type*;
        using difference_type = ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

        explicit ptrarray_iterator(pointer buff) : m_buff(buff)
        {}

        auto& operator ++ ()
        {
            m_buff++;
            return *this;
        }
        auto operator ++ (int)
        {
            auto tmp = ptrarray_iterator(*this);
            this->operator++();
            return tmp;
        }

        auto& operator -- ()
        {
            m_buff--;
            return *this;
        }
        auto operator -- (int)
        {
            auto tmp = ptrarray_iterator(*this);
            this->operator--();
            return tmp;
        }

        auto operator [] (difference_type n) -> value_type 
        {
            return m_buff[n];
        }

        auto& operator += (difference_type n)
        {
            m_buff += n;
            return *this;
        }
        auto& operator -= (difference_type n)
        {
            m_buff -= n;
            return *this;
        }

        auto operator + (difference_type n)
        {
            auto tmp = ptrarray_iterator(*this);
            return tmp += n;
        }

        auto operator - (difference_type n)
        {
            auto tmp = ptrarray_iterator(*this);
            return tmp -= n;
        }
        auto operator - (const ptrarray_iterator& rhs) -> difference_type
        {
            return m_buff - rhs.m_buff;
        }

        bool operator == (const ptrarray_iterator& rhs) const {
            return m_buff == rhs.m_buff;
        }
        bool operator != (const ptrarray_iterator& rhs) const {
            return !(*this == rhs);
        }
        bool operator < (const ptrarray_iterator& rhs) const {
            return m_buff < rhs.m_buff;
        }
        bool operator > (const ptrarray_iterator& rhs) const {
            return m_buff > rhs.m_buff;
        }
        bool operator >= (const ptrarray_iterator& rhs) const {
            return !(*this < rhs);
        }
        bool operator <= (const ptrarray_iterator& rhs) const {
            return !(*this > rhs);
        }

        auto operator * () { return *m_buff; }

    private:
        pointer m_buff;
    };

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



    template<class T>
    struct ci_char_traits : public std::char_traits<T> {
        using typename std::char_traits<T>::char_type;

        static char to_upper(char ch) {
            return static_cast<char>(toupper(static_cast<unsigned char>(ch)));
        }
        static wchar_t to_upper(wchar_t ch) {
            return towupper(ch);
        }

        static bool eq(char_type c1, char_type c2) {
            return to_upper(c1) == to_upper(c2);
        }
        static bool lt(char_type c1, char_type c2) {
            return to_upper(c1) < to_upper(c2);
        }
        static int compare(const char_type* s1, const char_type* s2, size_t n) {
            while (n-- != 0) {
                if (to_upper(*s1) < to_upper(*s2)) return -1;
                if (to_upper(*s1) > to_upper(*s2)) return 1;
                ++s1; ++s2;
            }
            return 0;
        }
        static const char_type* find(const char_type* s, int n, char_type a) {
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

    using ci_string_view = std::basic_string_view<char, ci_char_traits<char>>;
	using ci_string = std::basic_string<char, ci_char_traits<char>>;

    using ci_wstring_view = std::basic_string_view<wchar_t, ci_char_traits<wchar_t>>;
    using ci_wstring = std::basic_string<wchar_t, ci_char_traits<wchar_t>>;



	class environ_cache
	{
		friend environment;

		using vector_t = std::vector<std::string>;

		using iterator = vector_t::iterator;
		using const_iterator = vector_t::const_iterator;

		environ_cache();
		~environ_cache() noexcept;

		environ_cache(const environ_cache&) = delete;

		// cache and os, lock
		std::string_view getvar(std::string_view);
		void setvar(std::string_view, std::string_view);
		void rmvar(std::string_view);

		// cache only, no lock
		const_iterator find(std::string_view) const noexcept;

		// os only, no lock
		bool contains(std::string_view) const;


		// cache only, no lock
		iterator getenvstr(std::string_view) noexcept;

		// cache and os, no lock
		iterator getenvstr_sync(std::string_view);

		// members
		std::mutex m_mtx;
		vector_t myenv;
	};


} // detail

} // red::session

#endif