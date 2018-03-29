#ifndef _SAMPLER_FLIP_H
#define _SAMPLER_FLIP_H

#include "sampler/unigram.hh"
#include "sampler/bigram.hh"


namespace sampler
{
    class batch_unigram_flip: public batch_unigram
    {
    public:
        batch_unigram_flip(data::data* constants);
        virtual ~batch_unigram_flip();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };

    class batch_bigram_flip: public batch_bigram
    {
    public:
        batch_bigram_flip(data::data* constants);
        virtual ~batch_bigram_flip();

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };
}

#endif  // _SAMPLER_FLIP_H
