#ifndef _ESTIMATOR_FLIP_H
#define _ESTIMATOR_FLIP_H

#include "estimator/unigram.hh"
#include "estimator/bigram.hh"


namespace estimator
{
    class batch_unigram_flip: public batch_unigram
    {
    public:
        batch_unigram_flip(const parameters& params, const text::corpus_base& corpus, const annealing& anneal);
        virtual ~batch_unigram_flip();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };

    class batch_bigram_flip: public batch_bigram
    {
    public:
        batch_bigram_flip(const parameters& params, const text::corpus_base& corpus, const annealing& anneal);
        virtual ~batch_bigram_flip();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };
}

#endif  // _ESTIMATOR_FLIP_H
