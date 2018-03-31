#ifndef _SAMPLER_BIGRAM_H
#define _SAMPLER_BIGRAM_H

#include "sampler/base.hh"


namespace sampler
{
    class bigram: public base
    {
    public:
        bigram(const data::data& constants);
        virtual ~bigram();

        virtual bool sanity_check() const;
        virtual F log_posterior() const;
        virtual Fs predict_pairs(const TestPairs& test_pairs) const;
        virtual void print_lexicon(std::wostream& os) const;

        virtual void estimate(U iters, std::wostream& os, U eval_iters = 0,
            F temperature = 1, bool maximize = false, bool is_decayed = false) = 0;

    protected:
        Unigrams _ulex;
        Bigrams _lex;

        virtual void print_statistics(std::wostream& os, U iters, F temp, bool do_header=false);
        virtual Bs hypersample(F temperature);
        virtual void estimate_sentence(Sentence& s, F temperature) = 0;
        virtual void estimate_eval_sentence(Sentence& s, F temperature, bool maximize = false);
    };


    class batch_bigram: public bigram
    {
    public:
        batch_bigram(const data::data& constants);
        virtual ~batch_bigram();
        virtual void estimate(
            U iters, std::wostream& os, U eval_iters = 0,
            F temperature = 1, bool maximize = false, bool is_decayed = false);

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature) = 0;
    };


    class online_bigram: public bigram
    {
    public:
        online_bigram(const data::data& constants, F forget_rate = 0);
        virtual ~online_bigram();
        virtual void estimate(
            U iters, std::wostream& os, U eval_iters = 0,
            F temperature = 1, bool maximize = false, bool is_decayed = false);

    protected:
        F _forget_rate;
        Sentences _sentences_seen;  // for use with DeacyedMCMC model in particular
        virtual void estimate_sentence(Sentence& s, F temperature) = 0;
        void forget_items(Sentences::iterator i);
    };
}


#endif  // _SAMPLER_BIGRAM_H
