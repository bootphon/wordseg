#ifndef _SAMPLER_TREE_H
#define _SAMPLER_TREE_H

#include "sampler/unigram.hh"
#include "sampler/bigram.hh"


namespace sampler
{
    class batch_unigram_tree: public batch_unigram
    {
    public:
        batch_unigram_tree(const parameters& params, const data::data& constants, const annealing& anneal);
        virtual ~batch_unigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, double temperature);
    };


    class online_unigram_tree: public online_unigram
    {
    public:
        online_unigram_tree(
            const parameters& params, const data::data& constants, const annealing& anneal, double forget_rate = 0);
        virtual ~online_unigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, double temperature);
    };

    class batch_bigram_tree: public batch_bigram
    {
    public:
        batch_bigram_tree(const parameters& params, const data::data& constants, const annealing& anneal);
        virtual ~batch_bigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, double temperature);
    };

    class online_bigram_tree: public online_bigram
    {
    public:
        online_bigram_tree(const parameters& params, const data::data& constants, const annealing& anneal);
        virtual ~online_bigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, double temperature);
    };
}


#endif  // _SAMPLER_TREE_H
