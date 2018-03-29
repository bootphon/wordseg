#ifndef _SCORING_H_
#define _SCORING_H_

#include <iostream>

#include "sentence.hh"
#include "base.hpp"
#include "sg_lexicon.hpp"

/*
  Scoring class calculates number of words correct
  in each sentence and keeps a running tally.
  Calculates precision and recall, and lexicon precision.
*/
class Scoring
{
    typedef size_t Count;
    typedef SGLexicon<S, Count> Lexicon;

public:
    Scoring();
    ~Scoring();

    double precision() const;
    double recall() const;
    double fmeas() const;
    double b_precision() const;
    double b_recall() const;
    double b_fmeas() const;
    double lexicon_precision() const;
    double lexicon_recall() const;
    double lexicon_fmeas() const;

    void reset();

    void print_results(std::wostream& os=std::wcout) const;
    void print_segmented_lexicon(std::wostream& os=std::wcout) const;
    void print_reference_lexicon(std::wostream& os=std::wcout) const;
    // void print_final_results(std::wostream& os=std::wcout) const;
    // void print_lexicon(const Lexicon& lexicon,wostream& os=std::wcout) const;
    // void print_lexicon_summary(const Lexicon& lexicon,wostream& os=std::wcout) const;

    int lexicon_correct() const;
    void add_words_to_lexicon(const Sentence::Words& words, Lexicon& lex);

    int _sentences;      // number of sentences so far in block
    int _words_correct;  // tokens
    int _segmented_words;
    int _reference_words;
    int _bs_correct;
    int _segmented_bs;
    int _reference_bs;
    Lexicon _segmented_lex;
    Lexicon _reference_lex;
};


#endif
