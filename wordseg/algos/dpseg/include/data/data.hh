#ifndef _DATA_BASE_H
#define _DATA_BASE_H

#include <unordered_map>
#include <vector>

#include "sentence.hh"
#include "substring.hh"

extern uint debug_level;

typedef std::unordered_map<substring, uint> S_U;
typedef std::vector<S_U> S_Us;
typedef std::pair<substring, substring> SS;
typedef std::vector<SS> TestPairs;


namespace data
{
    template<typename T>
    inline void error(const T& message)
    {
        std::cerr << "error: " << message << std::endl;
        abort();
        exit(1);
    }


    class data
    {
    public:
        // model configuration parameters
        bool do_mbdp;          // if true, compute using Brent model
        uint nchartypes;          //!< number of different char types in training data (used by monkey model)
        double Pstop;               //!< character (monkey) model parameter
        double aeos;                //!< Beta parameter for end-of-sentence
        double a1;                  //!< unigram PY parameters
        double b1;
        double a2;                  //!< bigram PY parameters
        double b2;
        double hypersampling_ratio; // the standard deviation for new hyperparm proposals
        double init_pboundary;      // initial prob of boundary
        double pya_beta_a;          // parm of beta prior on pya
        double pya_beta_b;          // parm of beta prior on pya
        double pyb_gamma_c;         // parm of gamma prior on pyb
        double pyb_gamma_s;         // parm of gamma prior on pyb
        // uint burnin_iterations;   //!< number of burn-in iterations
        // uint eval_iterations;     //!< number of posterior sampling iterations to evaluate on
        uint anneal_iterations;   //!< number of iterations to anneal for
        double anneal_start_temperature;  //!< start annealing at this temperature
        double anneal_stop_temperature;   //!< stop annealing at this temperature
        double anneal_a;            //!< a parameter in annealing sigmoid function
        double anneal_b;            //!< b parameter in annealing sigmoid function
        uint trace_every;         //!< Frequency with which tracing should be performed
        uint nparticles;          // number of particles in filter
        uint forget_rate;
        uint token_memory;
        uint type_memory;
        std::string forget_method;

        data();
        virtual ~data();

        const std::vector<unsigned int>& sentence_boundary_list() const;
        const std::vector<bool>& possible_boundaries() const;

        // may be needed for initializing boundaries in Sentence. Note
        // that for ExperimentalData, these will be empty.
        const std::vector<bool>& true_boundaries() const;

        uint nsentences() const;
        uint nchars() const;

        virtual void read(std::wistream& is, uint start, uint ns) = 0;

        std::vector<Sentence> get_sentences() const;

        // default is no eval sents, only CorpusData might have a real list
        virtual std::vector<Sentence> get_eval_sentences() const;

        // writes the data out segmented according to boundaries.
        std::wostream& write_segmented_corpus(
            const std::vector<bool>& b, std::wostream& os, int begin=0, int end=0) const;

        // returns the annealing temperature to be used at each
        // iteration.  If anneal_a is zero, we use Sharon Goldwater's
        // annealing schedule, where anneal_iterations are broken into
        // 9 equal sized bins, where the ith bin has temperature
        // 10/(bin+1).  If anneal_a is non-zero, we use a sigmoid
        // based annealing function.
        double anneal_temperature(uint iteration) const;

    protected:
        std::vector<unsigned int> sentenceboundaries;    //!< list of indices: first char of each sentence
        std::vector<bool> _possible_boundaries;  //!< locations to sample boundaries
        std::vector<bool> _true_boundaries;      //!< locations of gold boundaries

        uint ntrainsentences;     //!< number of sentences of training data to use
        uint ntrain;              //!< number of chars in training data

        virtual void initialize_chars();

        void initialize_boundaries(
            std::size_t start, std::size_t end,
            std::vector<bool>& possible_bs, std::vector<bool>& true_bs) const;
    };
}

#endif  // _DATA_BASE_H
