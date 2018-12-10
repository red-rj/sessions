#ifndef IXM_SESSION_HPP
#define IXM_SESSION_HPP

#include "session_impl.hpp"

namespace ixm::session {

    class environment
    {
    public:
        class variable
        {
        public:
            using path_iterator = detail::pathsep_iterator<char>;
        
            operator std::string_view() const noexcept;
            variable& operator = (std::string_view);
            std::string_view key() const noexcept { return m_key; }
            std::pair<path_iterator, path_iterator> split () const;

            explicit variable(std::string_view key_) : m_key(key_) {}
        private:
            std::string m_key;
        };

        using iterator = detail::charbuff_iterator;
        //using value_range = /* implementation-defined */;
        //using key_range = /* implementation-defined */;
        using value_type = variable;
        using size_type = size_t;


        template <class T>
        using ConvertsToSV_Only = std::enable_if_t<
            std::conjunction_v<
                std::is_convertible<const T&, std::string_view>, 
                std::negation<std::is_convertible<const T&, const char*>
                >
            >
        >;

        template <class T>
        using ConvertsToSV = std::enable_if_t<std::is_convertible_v<const T&, std::string_view>>;


        template <class T, class = ConvertsToSV_Only<T>>
        variable operator [] (T const& k) const {
            return operator[](k);
        }

        variable operator [] (std::string const&) const noexcept;
        variable operator [] (std::string_view) const;
        variable operator [] (char const*) const noexcept;

        template <class K, class = ConvertsToSV<K>>
        iterator find(K const& key) const noexcept {
            using detail::ci_string_view;
            
            ci_string_view keysv = key;
            
            for(auto it = cbegin(); it != cend(); it++)
            {
                ci_string_view elem = *it;

                if (elem.length() <= keysv.length()) continue;
                if (elem[keysv.length()] != '=') continue;
                if (elem.compare(0, keysv.length(), keysv) != 0) continue;
                
                return it;
            }
            
            return cend();
        }

        bool contains(std::string_view) const noexcept;

        iterator cbegin() const noexcept;
        iterator cend() const noexcept;

        iterator begin() const noexcept { return cbegin(); }
        iterator end() const noexcept { return cend(); }

        size_type size() const noexcept;
        bool empty() const noexcept { return size() == 0; }

        //value_range values() const noexcept;
        //key_range keys() const noexcept;

        template <class K, class = ConvertsToSV<K>>
        void erase(K const& key) noexcept {
            std::string keystr{key};
            internal_erase(keystr.c_str());
        }

    private:
        void internal_erase(const char*) noexcept;
    };



    class arguments
    {
    public:
        using iterator = detail::charbuff_iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using value_type = std::string_view;
        using index_type = size_t;
        using size_type = size_t;

        value_type operator [] (index_type) const noexcept;
        value_type at(index_type) const;

        [[nodiscard]] bool empty() const noexcept;
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
