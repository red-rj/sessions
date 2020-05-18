#include <algorithm>
#include <vector>
#include <memory>
#include <system_error>
#include <cstdlib>
#include <cstring>

#if defined(WIN32)
    #define _CRT_SECURE_NO_WARNINGS
    #include "win32.hpp"
    #include <shellapi.h>
#else

#endif // WIN32

#include "session.hpp"
#include "session_impl.hpp"

// helpers
namespace
{
char const** sys_argv() noexcept;
int sys_argc() noexcept;
char** sys_environ() noexcept;

extern const char sys_path_sep;

int osenv_find_pos(const char*);

template<class T>
size_t osenv_size(T** envptr) {
    size_t size = 0;
    while (envptr[size]) size++;
    return size;
}

std::string make_envstr(std::string_view k, std::string_view v)
{
    std::string es;
    es.reserve(k.size() + v.size() + 1);
    es += k; es += '='; es += v;
    return es;
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
        auto const ua = to_upper(a);
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
} // unnamed namespace

#if defined(WIN32)
namespace
{
    [[noreturn]]
    void throw_win_error(DWORD error = GetLastError())
    {
        throw std::system_error(static_cast<int>(error), std::system_category());
    }

    bool is_valid_utf8(const char* str, int str_l = -1) {
        return MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, 
            str, str_l, 
            nullptr, 0) != 0;
    }

    auto narrow(wchar_t const* wstr, int wstr_l = -1, char* ptr = nullptr, int length = 0) {
        return WideCharToMultiByte(
            CP_UTF8, 0, 
            wstr, wstr_l,
            ptr, length, 
            nullptr, nullptr);
    }

    auto wide(const char* nstr, int nstr_l = -1, wchar_t* ptr = nullptr, int length = 0) {
        const int CP = is_valid_utf8(nstr, nstr_l) ? CP_UTF8 : CP_ACP;
        return MultiByteToWideChar(
            CP, 0,
            nstr, nstr_l,
            ptr, length);
    }
    

    std::string to_utf8(std::wstring_view wstr) {
        auto length = narrow(wstr.data(), wstr.size());
        auto str8 = std::string(length, '*');
        auto result = narrow(wstr.data(), wstr.size(), str8.data(), length);

        if (result == 0)
            throw_win_error();

        return str8;
    }
    auto to_utf8_p(std::wstring_view wstr) {
        auto length = narrow(wstr.data(), wstr.size());
        auto sptr = std::make_unique<char[]>(length);
        auto result = narrow(wstr.data(), wstr.size(), sptr.get(), length);
        if (result == 0)
            throw_win_error();
        return sptr;
    }

    std::wstring to_utf16(std::string_view nstr) {
        auto length = wide(nstr.data(), nstr.size());
        auto str16 = std::wstring(length, L'*');
        auto result = wide(nstr.data(), nstr.size(), str16.data(), length);

        if (result == 0)
            throw_win_error();

        return str16;
    }

    auto init_args() {
        int argc;
        auto wargv = std::unique_ptr<LPWSTR[], decltype(LocalFree)*>{
            CommandLineToArgvW(GetCommandLineW(), &argc),
            LocalFree
        };

        if (!wargv)
            throw_win_error();
        
        std::vector<char const*> vec;
        try
        {
            vec.reserve(argc+1);
            
            for (int i = 0; i < argc; i++)
            {
                auto length = narrow(wargv[i]);
                auto ptr = new char[length];
                vec.push_back(ptr);

                auto result = narrow(wargv[i], -1, ptr, length);
                if (result==0)
                    throw_win_error();
            }
            
            vec.emplace_back(); // terminating null
        }
        catch(...)
        {
            for (auto p : vec) delete[] p;
            std::throw_with_nested(std::runtime_error("Failed to create arguments"));
        }
        
        return vec;
    }

    auto init_env() {
        // make sure _wenviron is initialized
        // https://docs.microsoft.com/en-us/cpp/c-runtime-library/environ-wenviron?view=vs-2017#remarks
        if (!_wenviron) {
            _wgetenv(L"initpls");
        }

        std::vector<char*> myenv;

        try
        {
            wchar_t** wenv = _wenviron;
            size_t wenv_l = osenv_size(wenv);
            myenv.reserve(wenv_l);

            std::transform(wenv, wenv+wenv_l, back_inserter(myenv), [](wchar_t* envstr) {
                auto uptr = to_utf8_p(envstr);
                return uptr.release();
            });
        }
        catch(...)
        {
            for (auto p : myenv) delete[] p;
            std::throw_with_nested(std::runtime_error("Failed to create environment"));
        }
        
        return myenv;
    }

    auto& argvec() {
        static auto vec = init_args();
        return vec;
    }
    auto& envvec() {
        static auto vec = init_env();
        return vec;
    }

    char const** sys_argv() noexcept { return argvec().data(); }
    int sys_argc() noexcept { return static_cast<int>(argvec().size()) - 1; }
    char** sys_environ() noexcept { return envvec().data(); }
    const char sys_path_sep = ';';

    int osenv_find_pos(const char* k)
    {
        auto** wenvp = _wenviron;
        auto wkey = to_utf16(k);

        auto predicate = ci_envstr_finder<wchar_t>(wkey);

        for (int i = 0; wenvp[i]; i++)
        {
            if (predicate(wenvp[i])) return i;
        }

        return -1;
    }

} // unnamed namespace
#elif defined(_POSIX_VERSION)
extern "C" char** environ;

namespace
{
    char const** my_argv{};
    int my_argc{};

    char const** sys_argv() noexcept { return my_argv; }
    int sys_argc() noexcept { return my_argc; }
    char** sys_environ() noexcept { return environ; }

} // unnamed namespace


namespace red::session
{
#ifndef SESSION_NOEXTENTIONS
    // https://gcc.gnu.org/onlinedocs/gcc-8.3.0/gcc/Common-Function-Attributes.html#index-constructor-function-attribute
    // https://stackoverflow.com/a/37012337
    [[gnu::constructor]]
#endif
    void init_args(int argc, const char** argv) {
        my_argv = argc;
        my_argc = argv;
    }
}
#endif


namespace red::session
{
    // args
    auto arguments::operator [] (arguments::index_type idx) const noexcept -> value_type
    {
        auto args = sys_argv();
        return args[idx];
    }

    arguments::value_type arguments::at(arguments::index_type idx) const
    {
        if (idx >= size()) {
            throw std::out_of_range("invalid arguments subscript");
        }

        auto args = sys_argv();
        return args[idx];
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
        return sys_argv();
    }

    int arguments::argc() const noexcept
    {
        return sys_argc();
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