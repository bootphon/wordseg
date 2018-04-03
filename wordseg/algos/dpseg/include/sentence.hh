#ifndef _TEXT_SENTENCE_H_
#define _TEXT_SENTENCE_H_

#include <iostream>
#include <vector>

#include "lexicon_items.hpp"
#include "scoring.hh"
#include "substring.hh"
#include "pitman_yor/unigrams.hpp"
#include "pitman_yor/bigrams.hpp"


typedef CharSeqLearned<substring> P0;
typedef pitman_yor::unigrams<P0> Unigrams;
typedef pitman_yor::bigrams<Unigrams> Bigrams;


class sentence: public substring
{
public:
    // initializes possible boundaries and actual boundaries.
    sentence(std::size_t start, std::size_t end,
             std::vector<bool>& possible_boundaries,
             std::vector<bool>& true_boundaries,
             std::size_t nsentences,
             double init_pboundary,
             double aeos);

    virtual ~sentence();

    std::vector<substring> get_segmented_words() const
        {
            return get_words(m_boundaries);
        }

    std::vector<substring> get_reference_words() const
        {
            return get_words(m_true_boundaries);
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

    std::size_t sample_by_flips(Unigrams& lex, double temperature);
    std::size_t sample_by_flips(Bigrams& lex, double temperature);

    // for DecayedMCMC
    void sample_one_flip(Unigrams& lex, double temperature, std::size_t boundary_within_sentence);
    void sample_one_flip(Bigrams& lex, double temperature, std::size_t boundary_within_sentence);

    void maximize(Unigrams& lex, std::size_t nsentences, double temperature, bool do_mbdp = 0);
    void maximize(Bigrams& lex, std::size_t nsentences, double temperature);

    void sample_tree(Unigrams& lex, std::size_t nsentences, double temperature, bool do_mbdp = 0);
    void sample_tree(Bigrams& lex, std::size_t nsentences, double temperature);

    void score(Scoring& scoring) const;

    friend std::wostream& operator<< (std::wostream& os, const sentence& s);

    std::vector<std::size_t> get_possible_boundaries()
        {
            return m_possible_boundaries;
        };

    const std::vector<bool>& boundaries() const
        {
            return m_boundaries;
        }

private:
    std::vector<bool> m_boundaries;
    std::vector<std::size_t> m_possible_boundaries;
    std::vector<std::size_t> m_padded_possible;  // for use with dynamic programming
    std::vector<bool> m_true_boundaries;

    double m_aeos;
    double m_init_pboundary;
    std::size_t m_nsentences;

    substring word_at(std::size_t left, std::size_t right) const;
    std::vector<substring> get_words(const std::vector<bool>& boundaries) const;

    void insert(std::size_t left, std::size_t right, Unigrams& lex) const;
    void erase(std::size_t left, std::size_t right, Unigrams& lex) const;
    void insert(std::size_t i0, std::size_t i1, std::size_t i2, Bigrams& lex) const;
    void erase(std::size_t i0, std::size_t i1, std::size_t i2, Bigrams& lex) const;

    double prob_boundary(
        std::size_t i1, std::size_t i, std::size_t i2,
        const Unigrams& lex, double temperature) const;

    double p_bigram(std::size_t i1, std::size_t i, std::size_t i2, const Bigrams& lex) const;

    double prob_boundary(
        std::size_t i0, std::size_t i1, std::size_t i, std::size_t i2, std::size_t i3,
        const Bigrams& lex, double temperature) const;

    void surrounding_boundaries(
        std::size_t i, std::size_t& i0, std::size_t& i1, std::size_t& i2, std::size_t& i3) const;

    double mbdp_prob(Unigrams& lex, const substring& word, std::size_t nsentences) const;
};


#endif  // _TEXT_SENTENCE_H_
