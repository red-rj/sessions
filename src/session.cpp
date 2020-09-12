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

#include "red/sessions/session.hpp"

using std::string;
using std::wstring;
using std::string_view;
using std::wstring_view;
namespace sys = red::session::sys;


// helpers
namespace
{
struct ci_char_traits : public std::char_traits<char> {
    using typename std::char_traits<char>::char_type;

    static bool eq(char_type c1, char_type c2) {
        return toupper(c1) == toupper(c2);
    }
    static bool lt(char_type c1, char_type c2) {
        return toupper(c1) < toupper(c2);
    }
    static int compare(const char_type* s1, const char_type* s2, size_t n) {
        while (n-- != 0) {
            if (toupper(*s1) < toupper(*s2)) return -1;
            if (toupper(*s1) > toupper(*s2)) return 1;
            ++s1; ++s2;
        }
        return 0;
    }
    static const char_type* find(const char_type* s, int n, char_type a) {
        auto const ua = toupper(a);
        while (n-- != 0)
        {
            if (toupper(*s) == ua)
                return s;
            s++;
        }
        return nullptr;
    }

private:
    static char toupper(char_type ch) {
        const auto& C = std::locale::classic();
        return std::toupper(ch, C);
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
    , bool>;

    StrView key;

    explicit envstr_finder_base(StrView k) : key(k) {}

    template<class T, Is_Other_Strview<T> = true>
    explicit envstr_finder_base(const T& k) : key(k.data(), k.size()) {}

    bool operator() (StrView entry) noexcept
    {
        return
            entry.length() > key.length() &&
            entry[key.length()] == '=' &&
            entry.compare(0, key.size(), key) == 0;
    }

    template<class T, Is_Other_Strview<T> = true>
    bool operator() (const T& v) noexcept {
        return this->operator()(StrView(v.data(), v.size()));
    }
};

using envstr_finder = envstr_finder_base<std::char_traits<char>>;
using ci_envstr_finder = envstr_finder_base<ci_char_traits>;
} // unnamed namespace

#if defined(WIN32)
#undef environ

#ifdef SESSIONS_UTF8
constexpr auto NARROW_CP = CP_UTF8;
#else
constexpr auto NARROW_CP = CP_ACP;
#endif // SESSIONS_UTF8

namespace
{
    [[noreturn]]
    void throw_win_error(DWORD error = GetLastError())
    {
        throw std::system_error(static_cast<int>(error), std::system_category());
    }


    auto wide(const char* nstr, int nstr_l = -1, wchar_t* ptr = nullptr, int length = 0) {
        return MultiByteToWideChar(
            NARROW_CP, 0,
            nstr, nstr_l,
            ptr, length);
    }

    auto narrow(wchar_t const* wstr, int wstr_l = -1, char* ptr = nullptr, int length = 0) {
        return WideCharToMultiByte(
            NARROW_CP, 0, 
            wstr, wstr_l,
            ptr, length, 
            nullptr, nullptr);
    }

    
    std::string to_narrow(std::wstring_view wstr) {
        auto length = narrow(wstr.data(), (int)wstr.size());
        auto str = std::string(length, '\0');
        auto result = narrow(wstr.data(), (int)wstr.size(), str.data(), length);
        str.shrink_to_fit();

        if (result == 0)
            throw_win_error();

        return str;
    }

    std::wstring to_wide(std::string_view nstr) {
        auto length = wide(nstr.data(), (int)nstr.size());
        auto str = std::wstring(length, L'\0');
        auto result = wide(nstr.data(), (int)nstr.size(), str.data(), length);
        str.shrink_to_fit();

        if (result == 0)
            throw_win_error();

        return str;
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
            vec.resize(argc+1, nullptr); // +1 for terminating null

            std::transform(wargv.get(), wargv.get()+argc, vec.begin(), [](const wchar_t* arg) {
                auto length = narrow(arg);
                auto* ptr = new char[length];
                auto result = narrow(arg, -1, ptr, length);
                if (result==0) {
                    delete[] ptr;
                    throw_win_error();
                }
                
                return ptr;
            });

            return vec;
        }
        catch(...)
        {
            for (auto p : vec) delete[] p;
            std::throw_with_nested(std::runtime_error("Failed to create arguments"));
        }
    }

    auto& argvec() {
        static auto vec = init_args();
        return vec;
    }
} // unnamed namespace

char const** sys::argv() noexcept { return argvec().data(); }
int sys::argc() noexcept { return static_cast<int>(argvec().size()) - 1; }

sys::env_t sys::envp() noexcept {
    return _wenviron;
}

string sys::getenv(string_view k) {
    auto wkey = to_wide(k);
    wchar_t* wval = _wgetenv(wkey.c_str());
    return wval ? to_narrow(wval) : "";
}
void sys::setenv(string_view key, string_view value) {
    auto wkey = to_wide(key);
    auto wvalue = to_wide(value);
    _wputenv_s(wkey.c_str(), wvalue.c_str());
}
void sys::rmenv(string_view k) {
    auto wkey = to_wide(k);
    _wputenv_s(wkey.c_str(), L"");
}

string sys::narrow(envchar const* s) {
    return s ? to_narrow(s) : "";
}

const char red::session::environment::path_separator = ';';

#elif defined(_POSIX_VERSION)
extern "C" char** environ;

namespace
{
    char const** my_argv{};
    int my_argc{};
} // unnamed namespace

#ifndef SESSION_NOEXTENTIONS
// https://gcc.gnu.org/onlinedocs/gcc-8.3.0/gcc/Common-Function-Attributes.html#index-constructor-function-attribute
// https://stackoverflow.com/a/37012337
[[gnu::constructor]]
void init_args(int argc, const char** argv) {
    my_argv = argc;
    my_argc = argv;
}
#else
void red::session::arguments::init(int argc, const char** argv) noexcept
{
    my_argv = argc;
    my_argc = argv;
}
#endif

char const** sys::argv() noexcept { return my_argv; }
int sys::argc() noexcept { return my_argc; }

sys::env_t sys::envp() noexcept {
    return environ;
}

string sys::getenv(string_view k) {
    string key{k};
    char* val = ::getenv(key.c_str());
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

const char red::session::environment::path_separator = ':';

#endif

namespace red::session
{
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

        return (*this)[idx];
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

    environment::variable::operator string() const {
        return sys::getenv(m_key);
    }

    auto environment::variable::operator= (string_view value) -> variable&
    {
        sys::setenv(m_key, value);
        return *this;
    }

    auto environment::variable::split() const->splitpath
    {
        auto v = sys::getenv(m_key);
        return splitpath{v};
    }

    auto environment::size() const noexcept -> size_type
    {
        //return ranges::distance(*this); //BUG: SIGSEGV - Stack overflow on "remove env. variables"
        return ranges::distance(begin(), end());
    }

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