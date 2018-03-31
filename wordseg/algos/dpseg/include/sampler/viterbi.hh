#ifndef _SAMPLER_VITERBI_H
#define _SAMPLER_VITERBI_H

#include "sampler/unigram.hh"
#include "sampler/bigram.hh"


namespace sampler
{
    class batch_unigram_viterbi: public batch_unigram
    {
    public:
        batch_unigram_viterbi(const data::data& constants);
        virtual ~batch_unigram_viterbi();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };


    class online_unigram_viterbi: public online_unigram
    {
    public:
        online_unigram_viterbi(const data::data& constants, F forget_rate = 0);
        virtual ~online_unigram_viterbi();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };


    class batch_bigram_viterbi: public batch_bigram
    {
    public:
        batch_bigram_viterbi(const data::data& constants);
        virtual ~batch_bigram_viterbi();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };


    class online_bigram_viterbi: public online_bigram
    {
    public:
        online_bigram_viterbi(const data::data& constants);
        virtual ~online_bigram_viterbi();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };
}

#endif  // _SAMPLER_VITERBI_H
