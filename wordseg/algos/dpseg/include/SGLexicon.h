#ifndef _SGLEXICON_H_
#define _SGLEXICON_H_

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <utility>  // for std::pair
#include <vector>


#ifdef NDEBUG
#define my_assert(X,Y) /* do nothing */
#else
#define my_assert(X,Y) if(!(X)){ std::wcerr << "data is " << (Y) << std::endl; assert(X); }
#endif


//key_type must have ==, <, and hash() (defined for most standard
// classes in Mark's utils.h) this base class assumes that datatype is
// numeric, although redefining inc and dec can change that.
template <typename key_type, typename data_type>
class SGLexiconBase: public std::unordered_map<key_type, data_type>
{
    typedef typename std::pair<data_type, data_type> dd_t;
    typedef typename std::unordered_map<typename SGLexiconBase::key_type, data_type> parent_t;

public:
    typedef typename std::pair<typename SGLexiconBase::key_type, data_type> value_type;
    typedef typename std::unordered_map<typename SGLexiconBase::key_type, data_type>::iterator iterator;
    typedef typename std::unordered_map<typename SGLexiconBase::key_type, data_type>::const_iterator const_iterator;
    typedef typename std::vector<value_type> LexVector;
    typedef typename std::vector<value_type>::iterator LexVectorIter;
    typedef typename std::vector<value_type>::const_iterator LexVectorCIter;

    SGLexiconBase() : _ntokens(0) {}

    virtual ~SGLexiconBase() {}

    virtual iterator begin() {return parent_t::begin();}
    virtual const_iterator begin() const {return parent_t::begin();}
    virtual iterator end() {return parent_t::end();}
    virtual const_iterator end() const {return parent_t::end();}

    virtual void check_invariant() const
        {
#ifndef NDEBUG
            data_type total(0);
            for (const_iterator i=begin(); i != end(); i++) {
                my_assert(i->second != 0, i->first);
                total += i->second;
            }
            my_assert((total == _ntokens), dd_t(total, _ntokens));
#endif
        }

    virtual void clear()
        {
            parent_t::clear();
            _ntokens = 0;
        }

    virtual size_t mem_size()
        {
            return parent_t::size();
        }

    virtual data_type ntokens() const
        {
            return _ntokens;
        }

    virtual size_t ntypes() const
        {
            return parent_t::size();
        }

    virtual data_type operator()(const typename SGLexiconBase::key_type& s) const
        {
            const_iterator i = this->find(s);
            if (i == end()) return 0;
            return i->second;
        }

    // return true if a new type was added
    // Unless redefined in subcalss, return value is 0/1 only.
    virtual size_t inc(const typename SGLexiconBase::key_type& s)
        {
            _ntokens++;
            if (this->find(s) != end())
            {
                (*this)[s]++;
                return 0;
            }

            parent_t::insert(std::pair<typename SGLexiconBase::key_type, data_type>(s,1));
            return 1;
        }

    //return true if a type was deleted
    virtual size_t dec(const typename SGLexiconBase::key_type& s)
        {
            (*this)[s]--;
            my_assert((*this)[s] >= 0, value_type(s,(*this)[s]));
            _ntokens--;
            if ((*this)[s] == 0)
            {
                this->erase(s);
                return 1;
            }
            return 0;
        }

    virtual LexVector sort_by_key() const
        {
            LexVector counts;
            for (const_iterator i=begin(); i != end(); i++)
            {
                counts.push_back(value_type(*i));
            }

            std::sort(counts.begin(), counts.end(), first_lessthan());
            return counts;
        }

    virtual LexVector sort_by_value() const
        {
            LexVector counts;
            for (const_iterator i=begin(); i != end(); i++)
            {
                counts.push_back(value_type(*i));
            }

            std::sort(counts.begin(), counts.end(), second_lessthan());
            return counts;
        }

    virtual void print_by_key(std::wostream& os=std::wcout) const
        {
            LexVector l = sort_by_key();
            for (LexVectorIter i=l.begin(); i != l.end(); i++)
            {
                os << *i << std::endl;
            }
            os << std::endl;
        }

    virtual void print_by_value(std::wostream& os=std::wcout) const
        {
            LexVector l = sort_by_value();
            for (LexVectorIter i=l.begin(); i != l.end(); i++)
            {
                os << *i << std::endl;
            }
            os << std::endl;
        }

    friend std::wostream& operator<< (std::wostream& os, const SGLexiconBase& lexicon)
        {
            for (const_iterator iter = lexicon.begin(); iter != lexicon.end(); iter++)
            {
                os << iter->first << " " << iter->second << std::endl;
            }
            os << "Total lexicon tokens: " << lexicon.ntokens() << std::endl;
            os << "Total lexicon types: " << lexicon.ntypes() << std::endl;
            return os;
        }

protected:
    data_type _ntokens;

    // don't accidentally use these from outside
    virtual data_type& operator[](const typename SGLexiconBase::key_type& s)
        {
            return parent_t::operator[](s);
        }

    virtual size_t size()
        {
            return parent_t::size();
        }

    struct first_lessthan
    {
        template <typename T1, typename T2>
        bool operator() (const T1& e1, const T2& e2)
            {
                return e1.first < e2.first;
            }
    };

    struct second_lessthan {
        template <typename T1, typename T2>
        bool operator() (const T1& e1, const T2& e2)
            {
                return e1.second < e2.second;
            }
    };
};


// The only difference between this and base class is that here we can
// increment by more than 1 at a time.  data_type must have ==, <, >,
// +=, and -= (i.e. numeric)
template <typename key_type, typename data_type>
class SGLexicon: public SGLexiconBase<key_type, data_type>
{
    typedef SGLexiconBase<typename SGLexicon::key_type, data_type> parent_t;

public:
    typedef typename std::pair<typename SGLexicon::key_type, data_type> value_type;
    typedef typename std::unordered_map<typename SGLexicon::key_type, data_type>::iterator iterator;
    typedef typename std::unordered_map<typename SGLexicon::key_type, data_type>::const_iterator const_iterator;

    SGLexicon() {}
    virtual ~SGLexicon() {}

    virtual iterator begin() {return parent_t::begin();}
    virtual const_iterator begin() const {return parent_t::begin();}
    virtual iterator end() {return parent_t::end();}
    virtual const_iterator end() const {return parent_t::end();}

    void test_function() {std::wcout << "got the function"  <<std::endl;}

    virtual size_t inc(const typename SGLexicon::key_type& s)
        {
            return parent_t::inc(s);
        }

    virtual size_t dec(const typename SGLexicon::key_type& s)
        {
            return parent_t::dec(s);
        }

    //return 1 if a new type was added, else 0.
    virtual size_t inc(const typename SGLexicon::key_type& s, data_type count)
        {
            parent_t::_ntokens += count;
            if (this->find(s) != end())
            {
                (*this)[s] += count;
                return 0;
            }

            parent_t::insert(std::pair<typename SGLexicon::key_type, data_type>(s,count));
            return 1;
        }

    //return true if a type was deleted
    virtual size_t dec(const typename SGLexicon::key_type& s, data_type count)
        {
            (*this)[s] -= count;
            my_assert((*this)[s] >= 0, value_type(s,(*this)[s]));
            parent_t::_ntokens -= count;
            if ((*this)[s] == 0)
            {
                this->erase(s);
                return 1;
            }
            return 0;
        }
};


#endif
