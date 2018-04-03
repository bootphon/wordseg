#include "substring.hh"

#include <cassert>


substring::substring()
{}

substring::substring(std::size_t start, std::size_t end)
    : _start(start),
      _length(end-start)
{
    assert(start < end);
    assert(end <= data.size());
}

substring::~substring()
{}

std::wstring substring::string() const
{
    return data.substr(_start, _length);
}

std::size_t substring::size() const
{
    return _length;
}

substring::iterator substring::begin()
{
    return data.begin()+_start;
}

substring::iterator substring::end()
{
    return begin()+_length;
}

substring::const_iterator substring::begin() const
{
    return data.begin()+_start;
}

substring::const_iterator substring::end() const
{
    return begin()+_length;
}

std::size_t substring::begin_index() const
{
    return _start;
}

std::size_t substring::end_index() const
{
    return _start+_length-1;
}

int substring::compare(const substring& s) const
{
    return data.compare(_start, _length, data, s._start, s._length);
}

bool substring::operator== (const substring& s) const
{
    return compare(s) == 0;
}

bool substring::operator!= (const substring& s) const
{
    return compare(s) != 0;
}

bool substring::operator< (const substring& s) const
{
    return compare(s) < 0;
}

size_t substring::hash() const
{
    std::size_t h = 0;
    std::size_t g;

    substring::const_iterator p = begin();
    substring::const_iterator end = p + _length;

    while (p != end)
    {
        h = (h << 4) + (*p++);
        if ((g = h&0xf0000000))
        {
            h = h ^ (g >> 24);
            h = h ^ g;
        }
    }

    return size_t(h);
}
