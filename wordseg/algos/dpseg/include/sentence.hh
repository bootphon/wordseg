#ifndef _SENTENCE_H_
#define _SENTENCE_H_

#include <iostream>
#include <vector>

#include "py/unigrams.hpp"
#include "base.hpp"


namespace data{class data;}
class Scoring;

typedef CharSeqLearned<substring> P0;
typedef py::unigrams<P0> Unigrams;
typedef py::bigrams<Unigrams> Bigrams;


class Sentence: public substring
{
public:
    typedef std::vector<substring> Words;
    typedef Words::const_iterator Words_iterator;

    Sentence(std::size_t start, std::size_t end,
             std::vector<bool>& possible_boundaries,
             std::vector<bool>& true_boundaries,
             std::size_t nsentences,
             double init_pboundary,
             double aeos);

    Words get_segmented_words() const
        {
            return get_words(_boundaries);
        }

    Words get_reference_words() const
        {
            return get_words(_true_boundaries);
        }

    std::wostream& print(std::wostream& os) const;

    // add counts of whole sentence
    void insert_words(Unigrams& lex) const;

    // remove counts of whole sentence
    void erase_words(Unigrams& lex);

    // add counts of whole sentence
    void insert_words(Bigrams& lex) const;

    // remove counts of whole sentence
    void erase_words(Bigrams& lex);

    uint sample_by_flips(Unigrams& lex, double temperature);
    uint sample_by_flips(Bigrams& lex, double temperature);

    // for DecayedMCMC
    void sample_one_flip(Unigrams& lex, double temperature, uint boundary_within_sentence);
    void sample_one_flip(Bigrams& lex, double temperature, uint boundary_within_sentence);

    void maximize(Unigrams& lex, uint nsentences, double temperature, bool do_mbdp = 0);
    void maximize(Bigrams& lex, uint nsentences, double temperature);

    void sample_tree(Unigrams& lex, uint nsentences, double temperature, bool do_mbdp = 0);
    void sample_tree(Bigrams& lex, uint nsentences, double temperature);

    void score(Scoring& scoring) const;

    friend std::wostream& operator<< (std::wostream& os, const Sentence& s);

    std::vector<unsigned int> get_possible_boundaries()
        {
            return _possible_boundaries;
        };

    std::vector<bool> _boundaries;

private:
    std::vector<unsigned int> _possible_boundaries;
    std::vector<unsigned int> _padded_possible;  // for use with dynamic programming
    std::vector<bool> _true_boundaries;

    double m_aeos;
    double m_init_pboundary;
    std::size_t m_nsentences;

    substring word_at(uint left, uint right) const
        {
            return substring(left + begin_index(), right + begin_index());
        }

    Words get_words(const std::vector<bool>& boundaries) const;
    void insert(uint left, uint right, Unigrams& lex) const;
    void erase(uint left, uint right, Unigrams& lex) const;
    void insert(uint i0, uint i1, uint i2, Bigrams& lex) const;
    void erase(uint i0, uint i1, uint i2, Bigrams& lex) const;
    double prob_boundary(uint i1, uint i, uint i2, const Unigrams& lex, double temperature) const;
    double p_bigram(uint i1, uint i, uint i2, const Bigrams& lex) const;
    double prob_boundary(uint i0, uint i1, uint i, uint i2, uint i3, const Bigrams& lex, double temperature) const;
    void surrounding_boundaries(uint i, uint& i0, uint& i1, uint& i2, uint& i3) const;
    double mbdp_prob(Unigrams& lex, const substring& word, uint nsentences) const;
};


typedef std::vector<Sentence> Sentences;


#endif
