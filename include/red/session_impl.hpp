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
		std::string_view getvar(const std::string&);
		void setvar(const std::string&, const std::string&);
		void rmvar(const std::string&);

		// cache only, no lock
		const_iterator find(const std::string&) const noexcept;

		// os only, no lock
		bool contains(const std::string&) const;


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