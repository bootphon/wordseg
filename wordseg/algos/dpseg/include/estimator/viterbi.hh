#ifndef _ESTIMATOR_VITERBI_H
#define _ESTIMATOR_VITERBI_H

#include "estimator/unigram.hh"
#include "estimator/bigram.hh"


namespace estimator
{
    class batch_unigram_viterbi: public batch_unigram
    {
    public:
        batch_unigram_viterbi(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal);
        virtual ~batch_unigram_viterbi();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };


    class online_unigram_viterbi: public online_unigram
    {
    public:
        online_unigram_viterbi(
            const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal, double forget_rate = 0);
        virtual ~online_unigram_viterbi();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };


    class batch_bigram_viterbi: public batch_bigram
    {
    public:
        batch_bigram_viterbi(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal);
        virtual ~batch_bigram_viterbi();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };


    class online_bigram_viterbi: public online_bigram
    {
    public:
        online_bigram_viterbi(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal);
        virtual ~online_bigram_viterbi();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };
}

#endif  // _ESTIMATOR_VITERBI_H
