#include "substring.hh"

#include <cassert>
#include <boost/functional/hash.hpp>

// global data object, which holds training and eval data
std::wstring substring::m_data;


substring::substring()
    : m_start(0), m_length(0)
{}


substring::substring(const substring& other)
    : m_start(other.m_start), m_length(other.m_length)
{}


substring::substring(std::size_t start, std::size_t end)
    : m_start(start),
      m_length(end - start)
{
    assert(start < end);
    assert(m_end <= data.size());
}

substring::~substring()
{}

const std::wstring& substring::data()
{
    return m_data;
}

void substring::data(const std::wstring& data)
{
    m_data = data;
}

std::wstring substring::string() const
{
    return m_data.substr(m_start, m_length);
}

std::size_t substring::size() const
{
    return m_length;
}

substring::iterator substring::begin()
{
    return m_data.begin() + m_start;
}

substring::iterator substring::end()
{
    return begin() + m_length;
}

substring::const_iterator substring::begin() const
{
    return m_data.begin() + m_start;
}

substring::const_iterator substring::end() const
{
    return begin() + m_length;
}

std::size_t substring::begin_index() const
{
    return m_start;
}

std::size_t substring::end_index() const
{
    return m_start + m_length - 1;
}

int substring::compare(const substring& s) const
{
    return m_data.compare(m_start, m_length, m_data, s.m_start, s.m_length);
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
    boost::hash<std::pair<std::size_t, std::size_t> > hasher;
    return hasher(std::make_pair(m_start, m_length));
}
