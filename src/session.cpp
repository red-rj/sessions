#if defined(WIN32)
    #define _CRT_SECURE_NO_WARNINGS
    #include "win32.hpp"
    #include <shellapi.h>
#endif // WIN32

#include <algorithm>
#include <vector>
#include <memory>
#include <system_error>
#include <cstdlib>
#include <cstring>

#include <range/v3/algorithm.hpp>

#include "session.hpp"
#include "sys.hpp"

using std::string;
using std::wstring;
using std::string_view;
using std::wstring_view;
using namespace red::session::detail;

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
                auto length = WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
                auto ptr = new char[length];
                vec.push_back(ptr);

                auto result = WideCharToMultiByte(CP_ACP, 0, wargv[i], -1, ptr, length, nullptr, nullptr);
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

    auto& argvec() {
        static auto vec = init_args();
        return vec;
    }
} // unnamed namespace

char const** sys::argv() noexcept { return argvec().data(); }
int sys::argc() noexcept { return static_cast<int>(argvec().size()) - 1; }
char** sys::envp() noexcept { return _environ; }
const char sys::path_sep = ';';

void sys::setenv(string_view k, string_view v) {
    auto line = make_envstr(k, v);
    line[k.size()] = '\0';
    auto* key = line.c_str();
    auto* value = key+k.size()+1;
    _putenv_s(key, value);
}
void sys::rmenv(string_view k) {
    string key{k};
    _putenv_s(key.c_str(), "");
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
char** sys::envp() noexcept { return environ; }
const char sys::path_sep = ':';

void sys::setenv(string_view k, string_view v) {
    string key{k}, value{v};
    ::setenv(key.c_str(), value.c_str(), true);
}
void sys::rmenv(string_view k) {
    string key{k};
    unsetenv(key.c_str());
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

// sys common
string sys::getenv(string_view k) {
    string key{k};
    auto* var = ::getenv(key.c_str());
    return var ? var : "";
}

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

    const char** arguments::argv() const noexcept
    {
        return sys::argv();
    }

    int arguments::argc() const noexcept
    {
        return sys::argc();
    }

    // env
    auto environment::variable::operator=(std::string_view value)->variable&
    {
        sys::setenv(m_key, value);
        m_value = value;
        return *this;
    }

    auto environment::variable::split() const->std::pair<path_iterator, path_iterator>
    {
        return { path_iterator(m_value), path_iterator() };
    }

    string environment::variable::query()
    {
        return sys::getenv(m_key);
    }

    auto environment::env_range() noexcept 
    -> env_range_t
    {
        return env_range_t(sys::envp());
    }

    size_t environment::envsize() noexcept { return sys::envsize(sys::envp()); }

    environment::environment() noexcept {
#ifdef WIN32
        if (!_environ)
            getenv("initpls");
#endif
    }

    auto environment::find(string_view k) const noexcept->iterator
    {
        using namespace ranges;
        auto rng = env_range();
#ifdef WIN32
        auto pred = ci_envstr_finder(k);
#else
        auto pred = envstr_finder(k);
#endif
        auto view = find_if(rng, pred);
        return view;
    }

    bool environment::contains(string_view k) const
    {
        string key{k};
        return ::getenv(key.c_str());
    }

    void environment::erase(string_view k)
    {
        sys::rmenv(k);
    }


    void environment::variable::path_iterator::next() noexcept
    {
        auto pos = m_var.find(sys::path_sep, m_offset);
        if (pos == string::npos) {
            m_current = m_var.substr(m_offset, pos);
            m_offset = pos;
        } else {
            m_current = m_var.substr(m_offset, pos - m_offset);
            m_offset = pos + 1;
        }
    }
    void environment::variable::path_iterator::prev() noexcept
    {
        auto pos = m_var.rfind(sys::path_sep, m_offset);
        if (pos != string::npos) {
            m_current = m_var.substr(pos, m_offset);
            m_offset = pos;
        }
    }

    
}