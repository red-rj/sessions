#if defined(WIN32)
#   define _CRT_SECURE_NO_WARNINGS
#   include "win32.hpp"
#   include <shellapi.h>
#elif defined(__unix__)
#   include <unistd.h>
#   include <fstream>
#   include <memory>
#endif
#include <vector>
#include <locale>
#include <system_error>
#include <cstdlib>
#include <cassert>
#include <range/v3/algorithm.hpp>
#include "red/sessions/session.hpp"

using std::string; using std::wstring;
using std::string_view; using std::wstring_view;

// system layer
namespace sys {
    using red::session::detail::envchar;
    using red::session::detail::envblock;

    envblock envp() noexcept;

    std::string getenv(std::string_view key);
    void setenv(std::string_view key, std::string_view value);
    void rmenv(std::string_view key);
    
} // namespace sys

// helpers
namespace {

template<typename Traits>
struct envstr_finder
{
    using char_type = typename Traits::char_type;
    using StrView = std::basic_string_view<char_type, Traits>;
    StrView key;

    explicit envstr_finder(StrView k) : key(k) {}

    CPP_template(class T)
        (requires !concepts::convertible_to<T, StrView>)
    explicit envstr_finder(const T& k) : key(k.data(), k.size())
    {}

    bool operator() (StrView entry) noexcept
    {
        return
            entry.length() > key.length() &&
            entry[key.length()] == '=' &&
            entry.compare(0, key.length(), key) == 0;
    }

    CPP_template(class T)
        (requires !concepts::convertible_to<T, StrView>)
    bool operator() (const T& v) noexcept {
        return this->operator()(StrView(v.data(), v.size()));
    }
};

} // unnamed namespace

#if defined(WIN32)
#undef environ

constexpr auto NARROW_CP =
#ifdef SESSIONS_UTF8
    CP_UTF8;
#else
    CP_ACP;
#endif // SESSIONS_UTF8

struct ci_char_traits : public std::char_traits<char> {
    using typename std::char_traits<char>::char_type;

    static bool eq(char_type c1, char_type c2) {
        auto& LC = std::locale::classic();
        return toupper(c1, LC) == toupper(c2, LC);
    }
    static bool lt(char_type c1, char_type c2) {
        auto& LC = std::locale::classic();
        return toupper(c1, LC) < toupper(c2, LC);
    }
    static int compare(const char_type* s1, const char_type* s2, size_t n) {
        auto& LC = std::locale::classic();
        while (n-- != 0) {
            if (toupper(*s1, LC) < toupper(*s2, LC)) return -1;
            if (toupper(*s1, LC) > toupper(*s2, LC)) return 1;
            ++s1; ++s2;
        }
        return 0;
    }
    static const char_type* find(const char_type* s, int n, char_type a) {
        auto& LC = std::locale::classic();
        auto const ua = toupper(a, LC);
        while (n-- != 0)
        {
            if (toupper(*s, LC) == ua)
                return s;
            s++;
        }
        return nullptr;
    }
};

using envfind_fn = envstr_finder<ci_char_traits>;

namespace {

    [[noreturn]]
    void throw_win_error(DWORD error = GetLastError())
    {
        throw std::system_error(static_cast<int>(error), std::system_category());
    }


    auto wide(const char* nstr, int nstr_l = -1, wchar_t* ptr = nullptr, int length = 0) {
        return MultiByteToWideChar(NARROW_CP, 0, nstr, nstr_l, ptr, length);
    }

    auto narrow(wchar_t const* wstr, int wstr_l = -1, char* ptr = nullptr, int length = 0) {
        return WideCharToMultiByte(NARROW_CP, 0, wstr, wstr_l, ptr, length, nullptr, nullptr);
    }

    template<class Ch, class Strview>
    auto convert_str(Strview instr) -> std::basic_string<Ch>
    {
        static_assert(!std::is_same_v<typename Strview::value_type, Ch>);

        auto convert = [instr](Ch* dst=nullptr, int length=0) {
            if constexpr (std::is_same_v<Ch, char>) {
                return narrow(instr.data(), (int)instr.size(), dst, length);
            }
            else if constexpr (std::is_same_v<Ch, wchar_t>) {
                return wide(instr.data(), (int)instr.size(), dst, length);
            }
        };

        auto length = convert();
        auto str = std::basic_string<Ch>(length, Ch(0));
        auto result = convert(str.data(), length);
        
        if (result == 0)
            throw_win_error();

        str.resize(result);
        return str;
    }

    std::string to_narrow(std::wstring_view wstr) {
        return convert_str<char>(wstr);
    }

    std::wstring to_wide(std::string_view nstr) {
        return convert_str<wchar_t>(nstr);
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
        catch (std::exception&)
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

sys::envblock sys::envp() noexcept {
    return _wenviron;
}

string sys::getenv(string_view k) {
    auto wkey = to_wide(k);
    auto* var = _wgetenv(wkey.c_str());
    if (var) {
        wstring wval = var;
        return to_narrow(wval);
    }
    else return {};
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

namespace red::session {

string detail::narrow_copy(envchar const* s) {
    return s ? to_narrow(s) : "";
}

const char** arguments::argv() const noexcept {
    return argvec().data();
}

int arguments::argc() const noexcept {
    return static_cast<int>(argvec().size()) - 1;
}

void arguments::init(int, const char**) noexcept {
    // noop
}

const char environment::path_separator = ';';

environment::environment() noexcept
{
    if (!_wenviron)
        _wgetenv(L"Red Sessions init wchar env");
}

} // namespace red::session

#elif defined(__unix__)

extern "C" char** environ;

static std::vector<const char*> myargs;

[[gnu::constructor]]
// must have external linkage
void sessions_autorun(int count, const char** args) {
    // std::copy(args, args+count, back_inserter(myargs));

    for (int i = 0; i<count; i++) {
        string_view item = args[i];
        auto a = new char[item.size()+1];
        item.copy(a, item.size());
        myargs.push_back(a);
    }
    
    myargs.push_back(nullptr);
}

using envfind_fn = envstr_finder<std::char_traits<char>>;

sys::envblock sys::envp() noexcept {
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

namespace red::session {

string detail::narrow_copy(envchar const* s) { 
    return s ? s : "";
}

arguments::arguments() 
{
#if defined(SESSIONS_NOEXTENTIONS) && HAS_PROCFS
    if (myargs.empty()) {
        std::ifstream proc{"/proc/self/cmdline"};

        if (proc.fail()) {
            return;
        }

        while (proc.good()) {
            string value;
            getline(proc, value, char(0));

            if (value.empty()) continue;

            auto bababooye = std::make_unique<char[]>(value.length()+1);
            value.copy(bababooye.get(), value.length());

            myargs.push_back(bababooye.release());
        }

        myargs.push_back(nullptr);
    }
#elif !defined(SESSIONS_NOEXTENTIONS)
    if (myargs.empty()) {
        throw std::logic_error("somehow 'myargs' is not initialized");
    }
#endif
}

const char** arguments::argv() const noexcept {
    return myargs.data();
}

int arguments::argc() const noexcept {
    return (int)myargs.size() - 1;
}


void arguments::init(int count, const char** args) noexcept
{
    if (!myargs.empty()) {
        myargs.clear();
    }
    
    std::copy(args, args+count, back_inserter(myargs));
    myargs.push_back(nullptr);
}

const char environment::path_separator = ':';

environment::environment() noexcept = default;

} // namespace red::session
#else
#   error "unknown platform"
#endif

// common
namespace red::session {

environment::variable::variable(std::string_view key_) : m_key(key_)
{
    m_value = sys::getenv(m_key);
}

auto environment::variable::operator= (string_view value) -> variable&
{
    sys::setenv(m_key, value);
    m_value = string(value);
    return *this;
}

auto environment::begin_cursor() const -> cursor
{
    return cursor(sys::envp());
}

auto environment::do_find(string_view k) const ->iterator
{
    return ranges::find_if(*this, envfind_fn(k));
}

bool environment::contains(string_view k) const
{
    return !sys::getenv(k).empty();
}

void environment::do_erase(string_view k)
{
    sys::rmenv(k);
}

} // namespace red::session