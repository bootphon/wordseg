#ifndef _DECAYED_MCMC_H
#define _DECAYED_MCMC_H

#include "sampler/unigram.hh"
#include "sampler/bigram.hh"


namespace sampler
{
    class dmcmc  // short for decayed MCMC
    {
    public:
        dmcmc(double decay_rate = 0, uint samples_per_utt = 100);
        virtual ~dmcmc();

    protected:
        double _decay_rate;
        uint _samples_per_utt;
        std::vector<double> _decay_offset_probs;
        double _cum_decay_prob;
        uint _num_total_pot_boundaries;
        uint _num_curr_pot_boundaries;
        std::vector<unsigned int> _boundaries_num_sampled;
        uint _boundary_within_sentence;
        Sentences::iterator _sentence_sampled;
        virtual void decayed_initialization(Sentences _sentences);
        virtual void calc_new_cum_prob(Sentence& s, uint num_boundaries);
        virtual uint find_boundary_to_sample();
        virtual void find_sent_to_sample(uint b_to_sample, Sentence& to_sample, Sentences& sentences_seen);
        void replace_sampled_sentence(Sentence s, Sentences& sentences_seen);
    };


    class online_unigram_dmcmc: public online_unigram, dmcmc
    {
    public:
        online_unigram_dmcmc(
            const parameters& params, const data::data& constants, const annealing& anneal,
            double forget_rate = 0, double decay_rate = 1.0, uint samples_per_utt = 1000);

        virtual ~online_unigram_dmcmc()
            {}

    protected:
        virtual void estimate_sentence(Sentence& s, double temperature);
    };


    class online_bigram_dmcmc: public online_bigram, dmcmc
    {
    public:
        online_bigram_dmcmc(
            const parameters& params, const data::data& constants, const annealing& anneal,
            double forget_rate = 0, double decay_rate = 1.0, uint samples_per_utt = 1000);

        virtual ~online_bigram_dmcmc()
            {}

    protected:
        virtual void estimate_sentence(Sentence& s, double temperature);
    };
}

#endif  // _DECAYED_MCMC_H
