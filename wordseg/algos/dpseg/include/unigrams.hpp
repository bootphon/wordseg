#ifndef _UNIGRAMS_H_
#define _UNIGRAMS_H_

#include <iostream>

#include "mhs.hpp"


template <typename Base>
class UnigramsT: public PYAdaptor<Base>
{
private:
    typedef PYAdaptor<Base> parent;
    typedef typename Base::argument_type V;

public:
    typedef std::unordered_map<V,typename parent::T> WordTypes;

    UnigramsT(Base& base, uniform01_type& u01, F a=0, F b=1)
        : parent(base, u01, a, b)
        {}

    const WordTypes& types()
        {
            return parent::label_tables;
        }

    std::wostream& print(std::wostream& os) const
        {
            os << "types = " << parent::ntypes()
               << ", tokens = " << parent::ntokens()
               << std::endl;

        wchar_t sep = '(';
        for(const auto& item: parent::label_tables)
        {
            os << sep << item.first << ' ';
            // item.second.n;
            sep = ',';
        }

        return os << "))";
    }
};


//a set of bigram rest's
template <typename Base>
class BigramsT: public std::unordered_map<typename Base::argument_type,PYAdaptor<Base> >
{
    typedef typename Base::argument_type V;
    typedef typename std::unordered_map<typename Base::argument_type,PYAdaptor<Base> > parent;

public:
    typedef PYAdaptor<Base> BigramR;  // single bigram restaurant
    typedef typename Base::argument_type argument_type;

    BigramsT(Base& u, uniform01_type& u01, F a=0, F b=1)
        : _base(u), _empty_bigram(_base, u01, a, b)
        {}

    const Base& base_dist() const
        {
            return _base;
        }

    F& pya()
        {
            return _empty_bigram.pya();
        }

    F& pyb()
        {
            return _empty_bigram.pyb();
        }

    F operator() (const V& w1, const V& w2) const
        {
            // if (debug_level >= 1000000) TRACE1(v);
            typename BigramsT::const_iterator it = this->find(w1);
            F prob;
            if (it == parent::end())
            {
                prob = _base(w2);
                if (debug_level >= 100000) TRACE2(w2,prob);
            }
            else
            {
                prob = it->second(w2);
                if (debug_level >= 100000) TRACE2(w2,prob);
            }

            return prob;
        }

    F insert(const V& w1, const V& w2)
        {
            assert(_empty_bigram.empty());

            // b will be _empty_bigram if no restaurant for w1,
            // otherwise the existing restaurant.
            BigramR& b = parent::insert(
                typename BigramsT::value_type(w1, _empty_bigram)).first->second;

            assert(&b.base_dist() == &_empty_bigram.base_dist());
            return b.insert(w2);
        }

    void erase(const V& w1, const V& w2)
        {
            typename BigramsT::iterator it = this->find(w1);
            assert(it != parent::end());
            it->second.erase(w2);
            if (it->second.empty())
                std::unordered_map<V,BigramR>::erase(it);
        }

    bool sanity_check() const
        {
            bool sane = true;
            for(const auto& item: *this)
            {
                sane = sane and item.second.sanity_check();
            }

            return sane;
        }

    friend std::wostream& operator<< (std::wostream& os, const BigramsT& b)
        {
            os << "unigrams: " << b._base << std::endl;

            for(const auto& i: b)
            {
                os << i.first << ": " << i.second << std::endl;
            }

            return os;
        }

private:
    Base& _base;
    BigramR _empty_bigram;
};


#endif
