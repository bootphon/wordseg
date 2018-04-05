#ifndef _CORPUS_CORPUS_DATA_HH
#define _CORPUS_CORPUS_DATA_HH

#include "corpus/corpus_base.hh"

namespace corpus
{
    class corpus_data: public corpus_base
    {
    public:
        corpus_data();
        virtual ~corpus_data();

        virtual void read(std::wistream& is, std::size_t start, std::size_t ns);

        // read additional data for evaluation
        void read_eval(std::wistream& is, std::size_t start, std::size_t ns);

        virtual std::vector<sentence> get_eval_sentences(double init_pboundary, double aeos) const;

        void initialize(std::size_t ns);

    private:
        std::size_t m_evalsent_start;  // sentence # of first eval sentence

        void read_data(std::wistream& is, std::size_t start, std::size_t ns);
    };
}

#endif  // _CORPUS_CORPUS_DATA_HH
