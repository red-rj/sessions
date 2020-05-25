#if defined(WIN32)
    #define _CRT_SECURE_NO_WARNINGS
    #include "win32.hpp"
    #include <shellapi.h>
#endif // WIN32

#include <vector>
#include <locale>
#include <system_error>
#include <cstdlib>
#include <cassert>

#include <range/v3/algorithm.hpp>

#include "red/session.hpp"

using std::string;
using std::wstring;
using std::string_view;
using std::wstring_view;
using namespace red::session::detail;
namespace sys = red::session::sys;


// helpers
namespace
{
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

    static bool eq(char_type c1, char_type c2) {
        std::locale l;
        return toupper(c1, l) == toupper(c2, l);
    }
    static bool lt(char_type c1, char_type c2) {
        std::locale l;
        return toupper(c1, l) < toupper(c2, l);
    }
    static int compare(const char_type* s1, const char_type* s2, size_t n) {
        std::locale l;
        while (n-- != 0) {
            if (toupper(*s1, l) < toupper(*s2, l)) return -1;
            if (toupper(*s1, l) > toupper(*s2, l)) return 1;
            ++s1; ++s2;
        }
        return 0;
    }
    static const char_type* find(const char_type* s, int n, char_type a) {
        std::locale l;
        auto const ua = toupper(a, l);
        while (n-- != 0)
        {
            if (toupper(*s, l) == ua)
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

using envstr_finder = envstr_finder_base<std::char_traits<char>>;

using ci_envstr_finder = envstr_finder_base<ci_char_traits<char>>;

} // unnamed namespace

#if defined(WIN32)
#undef environ
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
        auto length = narrow(wstr.data(), (int)wstr.size());
        auto str8 = std::string(length, '\3');
        auto result = narrow(wstr.data(), (int)wstr.size(), str8.data(), length);
        str8.shrink_to_fit();
        assert(str8.find('\3')==str8.npos);

        if (result == 0)
            throw_win_error();

        return str8;
    }

    std::wstring to_utf16(std::string_view nstr) {
        auto length = wide(nstr.data(), (int)nstr.size());
        auto str16 = std::wstring(length, L'\3');
        auto result = wide(nstr.data(), (int)nstr.size(), str16.data(), length);
        str16.shrink_to_fit();
        assert(str16.find(L'\3')==str16.npos);

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
        char* ptr;
        try
        {
            vec.reserve(argc+1);
            
            for (int i = 0; i < argc; i++)
            {
                auto length = narrow(wargv[i]);
                ptr = new char[length];
                auto result = narrow(wargv[i], -1, ptr, length);
                if (result==0) {
                    throw_win_error();
                }

                vec.push_back(ptr);
            }
            
            vec.emplace_back(); // terminating null
        }
        catch(...)
        {
            delete[] ptr;
            for (auto p : vec) delete[] p;
            std::throw_with_nested(std::runtime_error("Failed to create arguments"));
        }
        
        return vec;
    }

    auto& argvec() {
        static auto vec = init_args();
        return vec;
    }
} // unnamed namespace

char const** sys::argv() noexcept { return argvec().data(); }
int sys::argc() noexcept { return static_cast<int>(argvec().size()) - 1; }

string sys::getenv(string_view k) {
    auto wkey = to_utf16(k);
    wchar_t* wval = _wgetenv(wkey.c_str());
    return wval ? to_utf8(wval) : "";
}
void sys::setenv(string_view key, string_view value) {
    auto wenv = to_utf16(make_envstr(key, value));
    wenv[key.size()] = L'\0';
    wchar_t const *wkey = wenv.c_str();
    auto wvalue = wkey + key.size() + 1;
    _wputenv_s(wkey, wvalue);
}
void sys::rmenv(string_view k) {
    auto wkey = to_utf16(k);
    _wputenv_s(wkey.c_str(), L"");
}

string sys::narrow(envchar const* s) {
    return s ? to_utf8(s) : "";
}


#elif defined(_POSIX_VERSION)
extern "C" char** environ;

namespace
{
    char const** my_argv{};
    int my_argc{};
} // unnamed namespace

char const** sys::argv() noexcept { return my_argv; }
int sys::argc() noexcept { return my_argc; }

string sys::getenv(string_view k) {
    string key{k};
    char* val = getenv(key.c_str());
    return val ? val : "";
}
void sys::setenv(string_view k, string_view v) {
    string key{k}, value{v};
    ::setenv(key.c_str(), value.c_str(), true);
}
void sys::rmenv(string_view k) {
    string key{k};
    ::unsetenv(key.c_str());
}

string sys::narrow(envchar const* s) { 
    return s ? s : "";
}

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
namespace detail
{

} // namespace detail

    // args
    auto arguments::operator [] (arguments::index_type idx) const noexcept -> value_type
    {
        auto args = sys::argv();
        return args[idx];
    }

    arguments::value_type arguments::at(arguments::index_type idx) const
    {
        if (idx >= size()) {
            throw std::out_of_range("invalid arguments subscript");
        }

        auto args = sys::argv();
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

    const char** arguments::argv() const noexcept { return sys::argv(); }
    int arguments::argc() const noexcept { return sys::argc(); }

    // env

    auto environment::variable::split() const->splitpath_t
    {
        return splitpath_t{m_value};
    }

    size_t environment::size() const noexcept { return sys::envsize(sys::envp()); }

    environment::environment() noexcept
    {
#ifdef WIN32
        if (!_wenviron)
            _wgetenv(L"initpls");
#endif
    }

    auto environment::do_find(string_view k) const ->iterator
    {
#ifdef WIN32
        auto pred = ci_envstr_finder(k);
#else
        auto pred = envstr_finder(k);
#endif
        return ranges::find_if(*this, pred);
    }

    bool environment::contains(string_view k) const
    {
        return !sys::getenv(k).empty();
    }

    void environment::do_erase(string_view k)
    {
        sys::rmenv(k);
    }
}