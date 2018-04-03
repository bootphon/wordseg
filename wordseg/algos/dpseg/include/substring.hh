#ifndef _SUBSTRING_HH
#define _SUBSTRING_HH


#include <iostream>

// Class substring can be thought of as a string, but avoids copying
// characters all over by simply storing pointers to begin/end indices
// in the global string storing the entire data set.
class substring
{
public:
    typedef wchar_t value_type;
    typedef std::wstring::iterator iterator;
    typedef std::wstring::const_iterator const_iterator;

    static std::wstring data;

    substring();
    substring(std::size_t start, std::size_t end);
    ~substring();

    std::wstring string() const;
    std::size_t size() const;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    std::size_t begin_index() const;
    std::size_t end_index() const;

    int compare(const substring& s) const;

    bool operator== (const substring& s) const;
    bool operator!= (const substring& s) const;
    bool operator< (const substring& s) const;

    friend std::wostream& operator<< (std::wostream& os, const substring& s);
    std::size_t hash() const;

private:
    std::size_t _start;
    std::size_t _length;
};


namespace std
{
    template <> struct hash<substring> : public std::unary_function<substring, std::size_t>
    {
        std::size_t operator()(const substring& s) const
            {
                return s.hash();
            }
    };
}


#endif  // _SUBSTRING_HH
