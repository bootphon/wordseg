#ifndef _DATA_BASE_H
#define _DATA_BASE_H

#include <unordered_map>
#include <vector>

#include "typedefs.hh"
#include "sentence.hh"
#include "substring.hh"

extern U debug_level;

typedef std::unordered_map<substring, U> S_U;
typedef std::vector<S_U> S_Us;
typedef std::pair<substring, substring> SS;
typedef std::vector<SS> TestPairs;

namespace data
{
    template<typename String>
    inline void error(const String& s)
    {
        std::cerr << "error: " << s << std::endl;
        abort();
        exit(1);
    }


    class data
    {
    public:
        // model configuration parameters
        bool do_mbdp;          // if true, compute using Brent model
        U nchartypes;          //!< number of different char types in training data (used by monkey model)
        F Pstop;               //!< character (monkey) model parameter
        F aeos;                //!< Beta parameter for end-of-sentence
        F a1;                  //!< unigram PY parameters
        F b1;
        F a2;                  //!< bigram PY parameters
        F b2;
        F hypersampling_ratio; // the standard deviation for new hyperparm proposals
        F init_pboundary;      // initial prob of boundary
        F pya_beta_a;          // parm of beta prior on pya
        F pya_beta_b;          // parm of beta prior on pya
        F pyb_gamma_c;         // parm of gamma prior on pyb
        F pyb_gamma_s;         // parm of gamma prior on pyb
        U burnin_iterations;   //!< number of burn-in iterations
        U eval_iterations;     //!< number of posterior sampling iterations to evaluate on
        U anneal_iterations;   //!< number of iterations to anneal for
        F anneal_start_temperature;  //!< start annealing at this temperature
        F anneal_stop_temperature;   //!< stop annealing at this temperature
        F anneal_a;            //!< a parameter in annealing sigmoid function
        F anneal_b;            //!< b parameter in annealing sigmoid function
        U trace_every;         //!< Frequency with which tracing should be performed
        U nparticles;          // number of particles in filter
        U forget_rate;
        U token_memory;
        U type_memory;
        std::string forget_method;

        data();
        virtual ~data();

        const Us& sentence_boundary_list() const;
        const Bs& possible_boundaries() const;

        // may be needed for initializing boundaries in Sentence. Note
        // that for ExperimentalData, these will be empty.
        const Bs& true_boundaries() const;

        U nsentences() const;
        U nchars() const;

        virtual void read(std::wistream& is, U start, U ns) = 0;

        std::vector<Sentence> get_sentences() const;

        //default is no eval sents, only CorpusData might have a real list
        virtual std::vector<Sentence> get_eval_sentences() const;

        //! write_segmented_corpus() writes the data out segmented
        //! according to boundaries.
        std::wostream& write_segmented_corpus(
            const Bs& b, std::wostream& os, I begin=0, I end=0) const;

        //! anneal_temperature() returns the annealing temperature to be
        //! used at each iteration.  If anneal_a is zero, we use Sharon
        //! Goldwater's annealing schedule, where anneal_iterations are
        //! broken into 9 equal sized bins, where the ith bin has
        //! temperature 10/(bin+1).  If anneal_a is non-zero, we use a
        //! sigmoid based annealing function.
        F anneal_temperature(U iteration) const;

    protected:
        // Us goldboundaries;     //!< list of indices: first char of each gold word
        Us sentenceboundaries;    //!< list of indices: first char of each sentence
        Bs _possible_boundaries;  //!< locations to sample boundaries
        Bs _true_boundaries;      //!< locations of gold boundaries

        U ntrainsentences;     //!< number of sentences of training data to use
        U ntrain;              //!< number of chars in training data

        virtual void initialize_chars();

        void initialize_boundaries(U start, U end, Bs& possible_bs, Bs& true_bs) const;
    };
}

#endif  // _DATA_BASE_H
