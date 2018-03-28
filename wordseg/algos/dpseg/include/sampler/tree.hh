#ifndef _SAMPLER_TREE_H
#define _SAMPLER_TREE_H

#include "sampler/unigram.hh"
#include "sampler/bigram.hh"


namespace sampler
{
    class batch_unigram_tree: public batch_unigram
    {
    public:
        batch_unigram_tree(Data* constants);
        virtual ~batch_unigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };


    class online_unigram_tree: public online_unigram
    {
    public:
        online_unigram_tree(Data* constants, F forget_rate = 0);
        virtual ~online_unigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };

    class batch_bigram_tree: public batch_bigram
    {
    public:
        batch_bigram_tree(Data* constants);
        virtual ~batch_bigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };

    class online_bigram_tree: public online_bigram
    {
    public:
        online_bigram_tree(Data* constants);
        virtual ~online_bigram_tree();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };
}


#endif  // _SAMPLER_TREE_H
