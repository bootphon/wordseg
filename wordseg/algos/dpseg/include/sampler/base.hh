#ifndef _SAMPLER_BASE_H
#define _SAMPLER_BASE_H

#include <cstdlib>
#include <iostream>

#include "data.hh"
#include "scoring.hh"


template<typename T>
inline void error(const T& string)
{
    std::cerr << "error: " << string << std::endl;
    abort();
    exit(1);
}


template<typename T>
inline std::vector<T>& operator+= (std::vector<T>& a, const std::vector<T>& b)
{
    assert(a.size() == b.size());

    for (size_t i=0; i < a.size(); i++)
    {
        a[i] += b[i];
    }

    return a;
}


template<typename T>
inline std::vector<double> operator/ (const std::vector<T>& a, double b)
{
    std::vector<double> c(a.size());
    for (size_t i=0; i < a.size(); i++)
    {
        c[i] = a[i] / b;
    }

    return c;
}


// separator used to separate fields during printing of results
extern std::wstring sep;


namespace sampler
{
    class base
    {
    public:
        base(Data* constants);
        virtual ~base();

        virtual bool sanity_check() const;
        virtual F log_posterior() const = 0;

        //use whatever sampling method is in subclass to segment training data
        virtual void estimate(
            U iters, std::wostream& os, U eval_iters = 0,
            F temperature = 1, bool maximize = false, bool is_decayed = false) = 0;

        // make single pass through test data, segmenting based on sampling
        // or maximization of each utt, using current counts from training
        // data only (i.e. no new counts are added)
        virtual void run_eval(std::wostream& os, F temperature = 1, bool maximize = false);
        virtual Fs predict_pairs(const TestPairs& test_pairs) const = 0;

        virtual void print_segmented(std::wostream& os) const;
        virtual void print_eval_segmented(std::wostream& os) const;
        virtual void print_lexicon(std::wostream& os) const = 0;

        //recomputes and prints precision, recall, etc. on training and eval data
        void print_scores(std::wostream& os);
        void print_eval_scores(std::wostream& os);


    protected:
        P0 _base_dist;
        Data* _constants;
        Sentences _sentences;
        Sentences _eval_sentences;
        U _nsentences_seen;
        Scoring _scoring;

        void resample_pya(Unigrams& lex);
        void resample_pyb(Unigrams& lex);

        virtual Bs hypersample(Unigrams& lex, F temperature);
        virtual Bs hypersample(Unigrams& ulex, Bigrams& lex, F temperature);
        virtual bool sample_hyperparm(F& beta, bool is_prob, F temperature);

        F log_posterior(const Unigrams& lex) const;
        F log_posterior(const Unigrams& ulex, const Bigrams& lex) const;

        Fs predict_pairs(const TestPairs& test_pairs, const Unigrams& lex) const;
        Fs predict_pairs(const TestPairs& test_pairs, const Bigrams& lex) const;

        void print_segmented_sentences(std::wostream& os, const Sentences& sentences) const;
        void print_scores_sentences(std::wostream& os, const Sentences& sentences);

        virtual void print_statistics(std::wostream& os, U iters, F temp, bool do_header=false) = 0;
        virtual void estimate_sentence(Sentence& s, F temperature) = 0;
        virtual void estimate_eval_sentence(Sentence& s, F temperature, bool maximize = false) = 0;

    };
}

#endif  // _SAMPLER_BASE_H
