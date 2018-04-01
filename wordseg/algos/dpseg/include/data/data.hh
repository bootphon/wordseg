#ifndef _DATA_BASE_H
#define _DATA_BASE_H

#include <unordered_map>
#include <vector>
#include "sentence.hh"

extern std::size_t debug_level;


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
        data();
        virtual ~data();

        const std::vector<std::size_t>& sentence_boundary_list() const;
        const std::vector<bool>& possible_boundaries() const;

        // may be needed for initializing boundaries in Sentence. Note
        // that for ExperimentalData, these will be empty.
        const std::vector<bool>& true_boundaries() const;

        std::size_t nchartypes() const;
        std::size_t nsentences() const;
        std::size_t nchars() const;

        virtual void read(std::wistream& is, std::size_t start, std::size_t ns) = 0;

        std::vector<Sentence> get_sentences(double init_pboundary, double aeos) const;

        // default is no eval sents, only CorpusData might have a real list
        virtual std::vector<Sentence> get_eval_sentences() const;

        // writes the data out segmented according to boundaries.
        std::wostream& write_segmented_corpus(
            const std::vector<bool>& b, std::wostream& os, int begin=0, int end=0) const;

    protected:
        // list of indices: first char of each sentence
        std::vector<std::size_t> m_sentenceboundaries;

        // locations to sample boundaries
        std::vector<bool> m_possible_boundaries;

        // locations of gold boundaries
        std::vector<bool> m_true_boundaries;

        // number of different char types in training data (used by monkey model)
        std::size_t m_nchartypes;

        // number of sentences of training data to use
        std::size_t m_ntrainsentences;

        // number of chars in training data
        std::size_t m_ntrain;

        // initialize m_nchartypes
        virtual void initialize_chars();

        void initialize_boundaries(
            std::size_t start, std::size_t end,
            std::vector<bool>& possible_bs, std::vector<bool>& true_bs) const;
    };
}

#endif  // _DATA_BASE_H
