#ifndef IXM_SESSION_HPP
#define IXM_SESSION_HPP

#include "session_impl.hpp"
#include "session_envcache.hpp"

namespace ixm::session {

    class environment
    {
    public:
        class variable
        {
        public:
            using path_iterator = detail::pathsep_iterator;
        
            operator std::string_view() const;
            variable& operator = (std::string_view);
            std::string_view key() const noexcept { return m_key; }
            std::pair<path_iterator, path_iterator> split () const;

            explicit variable(std::string_view key_) : m_key(key_) {}
        private:
            std::string m_key;
        };

        //using value_range = /* implementation-defined */;
        //using key_range = /* implementation-defined */;

        using iterator = detail::environ_cache::iterator;
        using value_type = variable;
        using size_type = size_t;


        template <class T>
        using ConvertsToSV_Only = std::enable_if_t<
            std::conjunction_v<
                std::is_convertible<const T&, std::string_view>, 
                std::negation<std::is_convertible<const T&, const char*>>
            >
        >;

        template <class T>
        using ConvertsToSV = std::enable_if_t<std::is_convertible_v<const T&, std::string_view>>;


        template <class T, class = ConvertsToSV_Only<T>>
        variable operator [] (T const& k) const {
            return variable(k);
        }

        variable operator [] (std::string const&) const;
        variable operator [] (std::string_view) const;
        variable operator [] (char const*) const;

        template <class K, class = ConvertsToSV<K>>
        iterator find(K const& key) const {
            std::string keystr{key};
            return cache.find(keystr);
        }

        bool contains(std::string_view) const;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept;
        bool empty() const noexcept { return size() == 0; }

        //value_range values() const noexcept;
        //key_range keys() const noexcept;

        template <class K, class = ConvertsToSV<K>>
        void erase(K const& key) {
            std::string keystr{key};
            cache.rmvar(keystr);
        }

    private:
        static detail::environ_cache cache;
    };



    class arguments
    {
    public:
        using iterator = detail::ptrarray_iterator<char const* const>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using value_type = std::string_view;
        using index_type = size_t;
        using size_type = size_t;

        value_type operator [] (index_type) const noexcept;
        value_type at(index_type) const;

        [[nodiscard]] bool empty() const noexcept { return size() == 0; }
        size_type size() const noexcept;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        reverse_iterator crbegin() const noexcept { return reverse_iterator{ cend() }; }
        reverse_iterator crend() const noexcept { return reverse_iterator{ cbegin() }; }

        reverse_iterator rbegin() const noexcept { return crbegin(); }
        reverse_iterator rend() const noexcept { return crend(); }

        [[nodiscard]] const char** argv() const noexcept;
        [[nodiscard]] int argc() const noexcept;
    };

} /* namespace ixm::session */
    
#endif /* IXM_SESSION_HPP */
