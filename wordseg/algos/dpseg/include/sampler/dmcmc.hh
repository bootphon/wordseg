#ifndef _DECAYED_MCMC_H
#define _DECAYED_MCMC_H

#include "sampler/unigram.hh"
#include "sampler/bigram.hh"


namespace sampler
{
    class dmcmc  // short for decayed MCMC
    {
    public:
        dmcmc(F decay_rate = 0, uint samples_per_utt = 100);
        virtual ~dmcmc();

    protected:
        F _decay_rate;
        U _samples_per_utt;
        Fs _decay_offset_probs;
        F _cum_decay_prob;
        U _num_total_pot_boundaries;
        U _num_curr_pot_boundaries;
        Us _boundaries_num_sampled;
        U _boundary_within_sentence;
        Sentences::iterator _sentence_sampled;
        virtual void decayed_initialization(Sentences _sentences);
        virtual void calc_new_cum_prob(Sentence& s, U num_boundaries);
        virtual U find_boundary_to_sample();
        virtual void find_sent_to_sample(U b_to_sample, Sentence& to_sample, Sentences& sentences_seen);
        void replace_sampled_sentence(Sentence s, Sentences& sentences_seen);
    };

    class online_unigram_dmcmc: public online_unigram, dmcmc
    {
    public:
        online_unigram_dmcmc(
            Data* constants, F forget_rate = 0, F decay_rate = 1.0, U samples_per_utt = 1000);

        virtual ~online_unigram_dmcmc()
            {}

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };

    class online_bigram_dmcmc: public online_bigram, dmcmc
    {
    public:
        online_bigram_dmcmc(
            Data* constants, F forget_rate = 0, F decay_rate = 1.0, U samples_per_utt = 1000);

        virtual ~online_bigram_dmcmc()
            {}

    protected:
        virtual void estimate_sentence(Sentence& s, F temperature);
    };
}

#endif  // _DECAYED_MCMC_H
