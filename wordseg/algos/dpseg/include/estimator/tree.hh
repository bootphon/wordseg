#ifndef _ESTIMATOR_TREE_H
#define _ESTIMATOR_TREE_H

#include "estimator/unigram.hh"
#include "estimator/bigram.hh"


namespace estimator
{
    class batch_unigram_tree: public batch_unigram
    {
    public:
        batch_unigram_tree(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal);
        virtual ~batch_unigram_tree();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };


    class online_unigram_tree: public online_unigram
    {
    public:
        online_unigram_tree(
            const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal, double forget_rate = 0);
        virtual ~online_unigram_tree();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };

    class batch_bigram_tree: public batch_bigram
    {
    public:
        batch_bigram_tree(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal);
        virtual ~batch_bigram_tree();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };

    class online_bigram_tree: public online_bigram
    {
    public:
        online_bigram_tree(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal);
        virtual ~online_bigram_tree();

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };
}


#endif  // _ESTIMATOR_TREE_H
