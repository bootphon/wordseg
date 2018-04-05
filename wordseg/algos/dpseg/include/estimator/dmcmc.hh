#ifndef _ESTIMATOR_DECAYED_MCMC_H
#define _ESTIMATOR_DECAYED_MCMC_H

#include "estimator/unigram.hh"
#include "estimator/bigram.hh"


namespace estimator
{
    class dmcmc  // short for decayed MCMC
    {
    public:
        dmcmc(double decay_rate = 0, std::size_t samples_per_utt = 100);
        virtual ~dmcmc();

    protected:
        double m_decay_rate;
        std::size_t m_samples_per_utt;
        std::vector<double> m_decay_offset_probs;
        double m_cum_decay_prob;
        std::size_t m_num_total_pot_boundaries;
        std::size_t m_num_curr_pot_boundaries;
        std::vector<std::size_t> m_boundaries_num_sampled;
        std::size_t m_boundary_within_sentence;
        std::vector<sentence>::iterator m_sentence_sampled;
        virtual void decayed_initialization(std::vector<sentence> m_sentences);
        virtual void calc_new_cum_prob(sentence& s, std::size_t num_boundaries);
        virtual std::size_t find_boundary_to_sample();
        virtual void find_sent_to_sample(
            std::size_t b_to_sample, sentence& to_sample, std::vector<sentence>& sentences_seen);
        void replace_sampled_sentence(sentence s, std::vector<sentence>& sentences_seen);
    };


    class online_unigram_dmcmc: public online_unigram, dmcmc
    {
    public:
        online_unigram_dmcmc(
            const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal,
            double forget_rate = 0, double decay_rate = 1.0, std::size_t samples_per_utt = 1000);

        virtual ~online_unigram_dmcmc()
            {}

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };


    class online_bigram_dmcmc: public online_bigram, dmcmc
    {
    public:
        online_bigram_dmcmc(
            const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal,
            double forget_rate = 0, double decay_rate = 1.0, std::size_t samples_per_utt = 1000);

        virtual ~online_bigram_dmcmc()
            {}

    protected:
        virtual void estimate_sentence(sentence& s, double temperature);
    };
}

#endif  // _ESTIMATOR_DECAYED_MCMC_H
