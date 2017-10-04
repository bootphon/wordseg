#ifndef _BASE_H_
#define _BASE_H_

// Class S can be thought of as a string, but avoids copying
// characters all over by simply storing pointers to begin/end indices
// in the global string storing the entire data set.
//
// All other classes are various base distributions to generate
// lexical items.


#include <cassert>
#include <iostream>

#include "mhs.h"


extern uniform01_type<F> unif01;


class S
{
public:
    typedef wchar_t value_type;
    typedef std::wstring::iterator iterator;
    typedef std::wstring::const_iterator const_iterator;

    static std::wstring data;

    S()
        {}

    S(std::size_t start, std::size_t end)
        : _start(start),
          _length(end-start)
        {
            assert(start < end);
            assert(end <= data.size());
        }

    std::wstring string() const
        {
            return data.substr(_start, _length);
        }

    std::size_t size() const
        {
            return _length;
        }

    iterator begin()
        {
            return data.begin()+_start;
        }

    iterator end()
        {
            return begin()+_length;
        }

    const_iterator begin() const
        {
            return data.begin()+_start;
        }

    const_iterator end() const
        {
            return begin()+_length;
        }

    std::size_t begin_index() const
        {
            return _start;
        }

    std::size_t end_index() const
        {
            return _start+_length-1;
        }

    int compare(const S& s) const
        {
            return data.compare(_start, _length, data, s._start, s._length);
        }

    bool operator== (const S& s) const
        {
            return compare(s) == 0;
        }

    bool operator!= (const S& s) const
        {
            return compare(s) != 0;
        }

    bool operator< (const S& s) const
        {
            return compare(s) < 0;
        }

    friend std::wostream& operator<< (std::wostream& os, const S& s);

    size_t hash() const
        {
            size_t h = 0;
            size_t g;
            const_iterator p = begin();
            const_iterator end = p + _length;

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

private:
    std::size_t _start;
    std::size_t _length;
};


namespace std
{
    template <> struct hash<S> : public std::unary_function<S, std::size_t>
    {
        std::size_t operator()(const S& s) const
            {
                return s.hash();
            }
    };
}


// the template type is actually irrelevant for the probabilities...
template <typename X>
class UniformMultinomial
{
public:
    typedef X argument_type;
    typedef F result_type;

    UniformMultinomial(F dimensions)
        : _dimensions(dimensions),
          _prob(1.0/_dimensions),
          _nitems(0)
        {
            assert(_dimensions >0);
        }

    U dimensions() const
        {
            return _dimensions;
        }

    //! operator() returns the probability of an item
    F operator()(const X& v) const
        {
            return _prob;
        }

    void insert(const X& v)
        {
            ++_nitems;
        }

    void erase(const X& v)
        {
            --_nitems;
        }

    bool empty() const
        {
            return _nitems == 0;
        }

    void clear()
        {
            _nitems = 0;
        }

    F logprob() const
        {
            return _nitems * log(_prob);
        }

private:
    F _dimensions;  // dimensions of multinomial distribution
    F _prob;        // probability of each item (= 1/_dimensions)
    U _nitems;      // total number of items generated
};


//! CharSeq{} implements a geometric distribution over character
// sequences using a unigram character model, with uniform distr. on
// chars.
template <typename Xs>
struct CharSeq {
private:
    F p_nl;              //!< probability of end of word
    U nc;                //!< number of distinct characters
    U nchars;            //!< total number of characters generated
    U nstrings;          //!< total number of strings (words) generated

public:
    typedef Xs argument_type;
    typedef F  result_type;

    CharSeq(F p_nl, U nc)
        : p_nl(p_nl), nc(nc), nchars(), nstrings()
        {
            assert(p_nl > 0);
            assert(p_nl <= 1);
            assert(nc > 0);
        }

    F p_stop() const
        {
            return p_nl;
        }

    F& p_stop()
        {
            return p_nl;
        }

    U nchartypes() const
        {
            return nc;
        }

    U get_nchars()
        {
            return nchars;
        }

    U get_nstrings()
        {
            return nstrings;
        }

    //! operator() returns the probability of a substring.  We
    //! special-case the situation where the initial substring is the
    //! end marker (assumed to be the character in position 0).
    F operator()(const Xs& v) const
        {
            return v == Xs(0,1) ? p_nl : pow((1.0 - p_nl) / nc, v.size()) * p_nl;
        }

    void insert(const Xs& v)
        {
            ++nstrings;
            if (v != Xs(0,1))
                nchars += v.size();
        }

    void erase(const Xs& v)
        {
            --nstrings;
            if (v != Xs(0,1))
                nchars -= v.size();
        }

    bool empty() const
        {
            return nchars == 0 && nstrings == 0;
        }

    void clear()
        {
            nchars = nstrings = 0;
        }

    //! logprob() returns the log probability of the corpus
    F logprob() const {
        return nstrings * log(p_nl)          // probability of stopping
            + nchars * log((1.0-p_nl)/nc);   // probability of characters
    }
};


//! CharSeq0{} is like CharSeq, but over non-empty character sequences
template <typename Xs>
class CharSeq0
{
private:
    F p_nl;              //!< probability of end of word
    U nc;                //!< number of distinct characters
    U nchars;            //!< total number of characters generated
    U nstrings;          //!< # of words generated (# tables)

public:
    typedef Xs argument_type;
    typedef F  result_type;

    CharSeq0(F p_nl, U nc)
        : p_nl(p_nl), nc(nc), nchars(), nstrings()
        {
            assert(p_nl > 0);
            assert(p_nl <= 1);
            assert(nc > 0);
        }

    F p_stop() const
        {
            return p_nl;
        }

    F& p_stop()
        {
            return p_nl;
        }

    U nchartypes() const
        {
            return nc;
        }

    U get_nchars()
        {
            return nchars;
        }

    U get_nstrings()
        {
            return nstrings;
        }

    //! operator() returns the probability of a substring.
    F operator()(const Xs& v) const
        {
            assert(v.size() > 0);
            assert(*v.begin() != '\n');

            return (1.0/nc) * pow((1.0 - p_nl) / nc, v.size() - 1) * p_nl;
        }

    void insert(const Xs& v)
        {
            ++nstrings;
            if (v != Xs(0,1))
                nchars += v.size();
        }

    void erase(const Xs& v)
        {
            --nstrings;
            if (v != Xs(0,1))
                nchars -= v.size();
        }

    bool empty() const
        {
            return nchars == 0 and nstrings == 0;
        }

    void clear()
        {
            nchars = nstrings = 0;
        }

    //! logprob() returns the log probability of the corpus
    F logprob() const
        {
            return nstrings * log(p_nl)                     // probability of stopping
                + nstrings * log(1.0/nc)                    // probability of first chars
                + (nchars - nstrings) * log((1.0-p_nl)/nc); // probability of remaining chars
    }
};


// A unigram sequence of chars, but the probabilities of the chars are
// learned (i.e. we keep an adaptor for single chars)
template <typename Xs>
class CharSeqLearned
{
private:
    typedef UniformMultinomial<wchar_t> Char;
    typedef PYAdaptor<Char> CharProbs;

    F p_nl;      // dummy, for compatibility w/ CharSeq
    U nc;        //!< number of distinct characters
    U nstrings;  //!< # of words generated (# tables)
    F _a;        // PY parm for chars
    F _b;        // PY parm for chars
    F _logprob;
    Char _base;
    CharProbs _char_probs;

public:
    typedef Xs argument_type;
    typedef F  result_type;

    // number of characters in base includes end-of-word marker
    CharSeqLearned(F p_nl, U nc)
        : p_nl(-1), nc(nc), nstrings(), _a(0), _b(1.0), _logprob(0),
          _base(nc + 1), _char_probs(_base, unif01, _a, _b)
        {}

    // dummy, for compatibility w/ CharSeq
    F p_stop() const
        {
            return p_nl;
        }

    // dummy, for compatibility w/ CharSeq
    F& p_stop()
        {
            return p_nl;
        }

    U nchartypes() const
        {
            return nc;
        }

    U get_nstrings() {
        return nstrings;
    }

    //! operator() returns the probability of a substring.  TODO
    // sgwater: need to modify this to more correctly deal with
    // utterance boundaries in bigram model. (Also insert/delete)
    F operator()(const Xs& v) const
        {
            assert(v.size() > 0);
            // assert(*v.begin() != '\n'); //only holds for unigram model

            F prob = 1;
            for(const auto& i: v)
            {
                prob *= _char_probs(i);
            }

            if(*v.begin() != '\n')
            {
                prob *= _char_probs(' ');
            }

            if(debug_level >= 100000) TRACE2(v,prob);
            return prob;
        }

    void insert(const Xs& v)
        {
            ++nstrings;
            if (debug_level >= 125000) TRACE(_logprob);

            for(const auto& i: v)
            {
                _logprob += log(_char_probs.insert(i));
                if (debug_level >= 130000) TRACE(_logprob);
            }

            if (*v.begin() != '\n')
            {
                _logprob += log(_char_probs.insert(' '));
            }

            if (debug_level >= 125000) TRACE(_logprob);
        }

    void erase(const Xs& v)
        {
            --nstrings;
            for(const auto& i: v)
            {
                _char_probs.erase(i);
                _logprob -= log(_char_probs(i));
            }

            if (* v.begin() != '\n')
            {
                _char_probs.erase(' ');
                _logprob -= log(_char_probs(' '));
            }
        }

    bool empty() const
        {
            return _char_probs.empty();
        }

    void clear()
        {
            nstrings = 0;
            _char_probs.clear();
        }

    F logprob() const
        {
            return _logprob;
        }
};


// Learns a bigram distriburtion over chars to compute the string
// probability.
template <typename Xs>
class BigramChars
{
private:
    typedef UniformMultinomial<wchar_t> Char;
    typedef PYAdaptor<Char> UnigramChars;
    typedef BigramsT<UnigramChars> CharProbs;

private:
    F p_nl;  // dummy, for compatibility w/ CharSeq
    U nc;    // number of distinct characters
    F _a;    // PY parm for chars
    F _b;    // PY parm for chars
    F _logprob;
    Char _base;
    UnigramChars _unigrams;
    CharProbs _char_probs;

public:
    typedef Xs argument_type;
    typedef F  result_type;

    // number of characters in base includes end-of-word marker
    BigramChars(F p_nl, U nc)
        : p_nl(-1), nc(nc),  _a(0), _b(1.0), _logprob(0),
          _base(nc + 1),  _unigrams(_base, unif01, _a, _b),
          _char_probs(_unigrams, unif01, 0.0, 1.0)
        {}

    // dummy, for compatibility w/ CharSeq
    F p_stop() const
        {
            return p_nl;
        }

    // dummy, for compatibility w/ CharSeq
    F& p_stop()
        {
            return p_nl;
        }

    U nchartypes() const
        {
            return nc;
        }

    //! operator() returns the probability of a substring.  sgwater:
    // need to modify this to more correctly deal with utterance
    // boundaries in bigram model.  (Also insert/delete)
    F operator()(const Xs& v) const
        {
            assert(v.size() > 0);
            F prob = 1;

            if (*v.begin() == '\n')
            {
                prob *= _char_probs('\n','\n');
            }
            else
            {
                typename Xs::const_iterator i = v.begin();
                prob *= _char_probs(' ',*i);

                for (i = v.begin()+1; i < v.end(); i++)
                {
                    prob *= _char_probs(*(i-1), *i);
                }
                prob *= _char_probs(*(v.end()-1),' ');
            }

            if (debug_level >= 100000) TRACE2(v,prob);
            return prob;
        }

    void insert(const Xs& v)
        {
            if (debug_level >= 125000) TRACE(_logprob);
            if (* v.begin() == '\n')
            {
                _logprob += log(_char_probs.insert('\n','\n'));
            }
            else
            {
                typename Xs::const_iterator i = v.begin();
                _logprob += log(_char_probs.insert(' ',*i));
                if (debug_level >= 130000) TRACE(_logprob);

                for (i = v.begin()+1; i < v.end(); i++)
                {
                    _logprob += log(_char_probs.insert(*(i-1),*i));
                    if (debug_level >= 130000) TRACE(_logprob);
                }
                _logprob += log(_char_probs.insert(*(v.end()-1),' '));
            }

            if (debug_level >= 125000) TRACE(_logprob);
        }

    void erase(const Xs& v)
        {
            if (* v.begin() == '\n')
            {
                _char_probs.erase('\n','\n');
                _logprob -= log(_char_probs('\n','\n'));
            }
            else
            {
                typename Xs::const_iterator i = v.begin();
                _char_probs.erase(' ', *i);
                _logprob -= log(_char_probs(' ', *i));
                for (i = v.begin()+1; i < v.end(); i++)
                {
                    _char_probs.erase(*(i-1), *i);
                    _logprob -= log(_char_probs(*(i-1), *i));
                }

                _char_probs.erase(*(v.end()-1),' ');
                _logprob -= log(_char_probs(*(v.end()-1),' '));
            }
        }

    bool empty() const
        {
            return _char_probs.empty();
        }

    void clear()
        {
            _char_probs.clear();
        }

    F logprob() const
        {
            return _logprob;
        }
};


typedef CharSeqLearned<S> P0;
typedef UnigramsT<P0> Unigrams;
typedef BigramsT<Unigrams> Bigrams;


#endif  // _BASE_H_
