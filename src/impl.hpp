#ifndef SESSION_SRC_IMPL_HPP
#define SESSION_SRC_IMPL_HPP

#include <cstddef>
#include <mutex>

namespace impl {

char const* argv (std::size_t) noexcept;
char const** argv() noexcept;
int argc () noexcept;

extern const char path_sep;


int osenv_find_pos(const char*);
    
template<class T>
inline size_t osenv_size(T** envptr) {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}

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

template<typename CharTraits>
struct envstr_finder_base
{
    using char_type = typename CharTraits::char_type;
    using StrView = std::basic_string_view<char_type, CharTraits>;

	template <class T>
	using Is_Other_Strview = std::enable_if_t<
		std::conjunction_v<
			std::negation<std::is_convertible<const T&, const char_type*>>,
			std::negation<std::is_convertible<const T&, StrView>>
		>
	>;

    StrView key;

    explicit envstr_finder_base(StrView k) : key(k) {}

	template<class T, class = Is_Other_Strview<T>>
	explicit envstr_finder_base(const T& k) : key(k.data(), k.size()) {}

    bool operator() (StrView entry) noexcept
    {
        return
            entry.length() > key.length() &&
            entry[key.length()] == '=' &&
            entry.compare(0, key.size(), key) == 0;
    }

	template<class T, class = Is_Other_Strview<T>>
	bool operator() (const T& v) noexcept {
		return this->operator()(StrView(v.data(), v.size()));
	}

};

template<typename T>
using envstr_finder = envstr_finder_base<std::char_traits<T>>;

template<typename T>
using ci_envstr_finder = envstr_finder_base<ci_char_traits<T>>;

inline std::string make_envstr(std::string_view k, std::string_view v)
{
	std::string es;
	es.reserve(k.size() + v.size() + 1);
	es += k; es += '='; es += v;
	return es;
}

static std::mutex env_mtx;

} /* namespace impl */

#endif /* SESSION_SRC_IMPL_HPP */
