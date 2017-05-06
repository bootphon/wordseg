/*
  Copyright 2007 Mark Johnson
  Copyright 2009 Sharon Goldwater

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SCORING_H_
#define _SCORING_H_

#include <iostream>
#include "Sentence.h"
#include "Base.h"
#include "SGLexicon.h"

/*
  Scoring class calculates number of words correct
  in each sentence and keeps a running tally.
  Calculates precision and recall, and lexicon precision.
*/
class Scoring
{
public:
    Scoring()
        :
        _sentences(0), _words_correct(0),
        _segmented_words(0), _reference_words(0),
        _bs_correct(0), _segmented_bs(0), _reference_bs(0)
        {}

    double precision() const
        {
            return (double)_words_correct/_segmented_words;
        }

    double recall() const
        {
            return (double)_words_correct/_reference_words;
        }

    double fmeas() const
        {
            return 2*recall()*precision()/(recall()+precision());
        }

    double b_precision() const
        {
            return (double)_bs_correct/_segmented_bs;
        }

    double b_recall() const
        {
            return (double)_bs_correct/_reference_bs;
        }

    double b_fmeas() const
        {
            return 2*b_recall()*b_precision()/(b_recall()+b_precision());
        }

    double lexicon_precision() const
        {
            return (double)lexicon_correct()/_segmented_lex.ntypes();
        }

        double lexicon_recall() const
        {
            return (double)lexicon_correct()/_reference_lex.ntypes();
        }

    double lexicon_fmeas() const
        {
            return 2*lexicon_precision()*lexicon_recall() /
                (lexicon_precision() + lexicon_recall());
        }

    void reset();

    void print_results(std::wostream& os=std::wcout) const;
    void print_segmented_lexicon(std::wostream& os=std::wcout) const;
    void print_reference_lexicon(std::wostream& os=std::wcout) const;

    // void print_final_results(std::wostream& os=std::wcout) const;
    // void print_lexicon(const Lexicon& lexicon,wostream& os=std::wcout) const;
    // void print_lexicon_summary(const Lexicon& lexicon,wostream& os=std::wcout) const;
    // void print_segmentation_summary(std::wostream& os=std::wcout) const
    //     {
    //         print_lexicon_summary(_segmented_lex);
    //     }

    typedef size_t Count;
    typedef SGLexicon<S, Count> Lexicon;

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
