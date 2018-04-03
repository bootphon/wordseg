#ifndef _ESTIMATOR_BIGRAM_H
#define _ESTIMATOR_BIGRAM_H

#include "estimator/base.hh"


namespace estimator
{
    class bigram: public base
    {
    public:
        bigram(const parameters& params, const text::corpus_base& corpus, const annealing& anneal);
        virtual ~bigram();

        virtual bool sanity_check() const;
        virtual double log_posterior() const;
        virtual std::vector<double> predict_pairs(
            const std::vector<std::pair<substring, substring> >& test_pairs) const;
        virtual void print_lexicon(std::wostream& os) const;

        virtual void estimate(uint iters, std::wostream& os, uint eval_iters = 0,
            double temperature = 1, bool maximize = false, bool is_decayed = false) = 0;

    protected:
        Unigrams m_ulex;
        Bigrams m_lex;

        virtual void print_statistics(std::wostream& os, uint iters, double temp, bool do_header=false);
        virtual std::vector<bool> hypersample(double temperature);
        virtual void estimate_sentence(sentence& s, double temperature) = 0;
        virtual void estimate_eval_sentence(sentence& s, double temperature, bool maximize = false);
    };


    class batch_bigram: public bigram
    {
    public:
        batch_bigram(const parameters& params, const text::corpus_base& corpus, const annealing& anneal);
        virtual ~batch_bigram();
        virtual void estimate(
            uint iters, std::wostream& os, uint eval_iters = 0,
            double temperature = 1, bool maximize = false, bool is_decayed = false);

    protected:
        virtual void estimate_sentence(sentence& s, double temperature) = 0;
    };


    class online_bigram: public bigram
    {
    public:
        online_bigram(
            const parameters& params, const text::corpus_base& corpus, const annealing& anneal, double forget_rate = 0);
        virtual ~online_bigram();
        virtual void estimate(
            uint iters, std::wostream& os, uint eval_iters = 0,
            double temperature = 1, bool maximize = false, bool is_decayed = false);

    protected:
        double m_forget_rate;

        // for use with DeacyedMCMC model in particular
        std::vector<sentence> m_sentences_seen;

        virtual void estimate_sentence(sentence& s, double temperature) = 0;
        void forget_items(std::vector<sentence>::iterator i);
    };
}


#endif  // _ESTIMATOR_BIGRAM_H
