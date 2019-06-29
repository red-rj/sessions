#ifndef RED_SESSION_IMPL_HPP
#define RED_SESSION_IMPL_HPP

#include <type_traits>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace red::session
{
	class environment;

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
		vector_t myenv;
	};


} // detail

} // red::session

#endif