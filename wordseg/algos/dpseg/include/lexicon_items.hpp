#ifndef _LEXICON_ITEMS_H_
#define _LEXICON_ITEMS_H_

// various base distributions to generate lexical items.

#include <cassert>
#include <iostream>

#include "pitman_yor/adaptor.hpp"
#include "pitman_yor/unigrams.hpp"
#include "pitman_yor/bigrams.hpp"


extern uniform01_type unif01;


// the template type is actually irrelevant for the probabilities...
template <typename X>
class UniformMultinomial
{
public:
    typedef X argument_type;
    typedef double result_type;

    UniformMultinomial(double dimensions)
        : _dimensions(dimensions),
          _prob(1.0/_dimensions),
          _nitems(0)
        {
            assert(_dimensions > 0);
        }

    std::size_t dimensions() const
        {
            return _dimensions;
        }

    //! operator() returns the probability of an item
    double operator()(const X& v) const
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

    double logprob() const
        {
            return _nitems * log(_prob);
        }

private:
    double _dimensions;  // dimensions of multinomial distribution
    double _prob;        // probability of each item (= 1/_dimensions)
    std::size_t _nitems;      // total number of items generated
};


//! CharSeq{} implements a geometric distribution over character
// sequences using a unigram character model, with uniform distr. on
// chars.
template <typename Xs>
struct CharSeq {
private:
    double p_nl;              //!< probability of end of word
    std::size_t nc;                //!< number of distinct characters
    std::size_t nchars;            //!< total number of characters generated
    std::size_t nstrings;          //!< total number of strings (words) generated

public:
    typedef Xs argument_type;
    typedef double  result_type;

    CharSeq(double p_nl, std::size_t nc)
        : p_nl(p_nl), nc(nc), nchars(), nstrings()
        {
            assert(p_nl > 0);
            assert(p_nl <= 1);
            assert(nc > 0);
        }

    double p_stop() const
        {
            return p_nl;
        }

    double& p_stop()
        {
            return p_nl;
        }

    std::size_t nchartypes() const
        {
            return nc;
        }

    std::size_t get_nchars()
        {
            return nchars;
        }

    std::size_t get_nstrings()
        {
            return nstrings;
        }

    //! operator() returns the probability of a substring.  We
    //! special-case the situation where the initial substring is the
    //! end marker (assumed to be the character in position 0).
    double operator()(const Xs& v) const
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
    double logprob() const
        {
            return nstrings * log(p_nl)          // probability of stopping
                + nchars * log((1.0-p_nl)/nc);   // probability of characters
        }
};


//! CharSeq0{} is like CharSeq, but over non-empty character sequences
template <typename Xs>
class CharSeq0
{
private:
    double p_nl;              //!< probability of end of word
    std::size_t nc;                //!< number of distinct characters
    std::size_t nchars;            //!< total number of characters generated
    std::size_t nstrings;          //!< # of words generated (# tables)

public:
    typedef Xs argument_type;
    typedef double  result_type;

    CharSeq0(double p_nl, std::size_t nc)
        : p_nl(p_nl), nc(nc), nchars(), nstrings()
        {
            assert(p_nl > 0);
            assert(p_nl <= 1);
            assert(nc > 0);
        }

    double p_stop() const
        {
            return p_nl;
        }

    double& p_stop()
        {
            return p_nl;
        }

    std::size_t nchartypes() const
        {
            return nc;
        }

    std::size_t get_nchars()
        {
            return nchars;
        }

    std::size_t get_nstrings()
        {
            return nstrings;
        }

    //! operator() returns the probability of a substring.
    double operator()(const Xs& v) const
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
    double logprob() const
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
    typedef pitman_yor::adaptor<Char> CharProbs;

    double p_nl;      // dummy, for compatibility w/ CharSeq
    std::size_t nc;        //!< number of distinct characters
    std::size_t nstrings;  //!< # of words generated (# tables)
    double _a;        // PY parm for chars
    double _b;        // PY parm for chars
    double _logprob;
    Char _base;
    CharProbs _char_probs;

public:
    typedef Xs argument_type;
    typedef double  result_type;

    // number of characters in base includes end-of-word marker
    CharSeqLearned(double p_nl, std::size_t nc)
        : p_nl(-1), nc(nc), nstrings(), _a(0), _b(1.0), _logprob(0),
          _base(nc + 1), _char_probs(_base, unif01, _a, _b)
        {}

    // dummy, for compatibility w/ CharSeq
    double p_stop() const
        {
            return p_nl;
        }

    // dummy, for compatibility w/ CharSeq
    double& p_stop()
        {
            return p_nl;
        }

    std::size_t nchartypes() const
        {
            return nc;
        }

    std::size_t get_nstrings() {
        return nstrings;
    }

    //! operator() returns the probability of a substring.  TODO
    // sgwater: need to modify this to more correctly deal with
    // utterance boundaries in bigram model. (Also insert/delete)
    double operator()(const Xs& v) const
        {
            assert(v.size() > 0);
            // assert(*v.begin() != '\n'); //only holds for unigram model

            double prob = 1;
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

    double logprob() const
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
    typedef pitman_yor::adaptor<Char> UnigramChars;
    typedef pitman_yor::bigrams<UnigramChars> CharProbs;

private:
    double p_nl;  // dummy, for compatibility w/ CharSeq
    std::size_t nc;    // number of distinct characters
    double _a;    // PY parm for chars
    double _b;    // PY parm for chars
    double _logprob;
    Char _base;
    UnigramChars _unigrams;
    CharProbs _char_probs;

public:
    typedef Xs argument_type;
    typedef double  result_type;

    // number of characters in base includes end-of-word marker
    BigramChars(double p_nl, std::size_t nc)
        : p_nl(-1), nc(nc),  _a(0), _b(1.0), _logprob(0),
          _base(nc + 1),  _unigrams(_base, unif01, _a, _b),
          _char_probs(_unigrams, unif01, 0.0, 1.0)
        {}

    // dummy, for compatibility w/ CharSeq
    double p_stop() const
        {
            return p_nl;
        }

    // dummy, for compatibility w/ CharSeq
    double& p_stop()
        {
            return p_nl;
        }

    std::size_t nchartypes() const
        {
            return nc;
        }

    //! operator() returns the probability of a substring.  sgwater:
    // need to modify this to more correctly deal with utterance
    // boundaries in bigram model.  (Also insert/delete)
    double operator()(const Xs& v) const
        {
            assert(v.size() > 0);
            double prob = 1;

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

    double logprob() const
        {
            return _logprob;
        }
};


#endif  // _LEXICON_ITEMS_H_
