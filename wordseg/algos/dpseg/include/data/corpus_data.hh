#ifndef _DATA_CORPUS_HH
#define _DATA_CORPUS_HH

#include "data/data.hh"

namespace data
{
    class corpus_data: public data
    {
    public:
        corpus_data();
        virtual ~corpus_data();

        virtual void read(std::wistream& is, std::size_t start, std::size_t ns);

        // read additional data for evaluation
        void read_eval(std::wistream& is, std::size_t start, std::size_t ns);

        virtual std::vector<Sentence> get_eval_sentences(double init_pboundary, double aeos) const;

        void initialize(std::size_t ns);

    private:
        std::size_t _evalsent_start;  // sentence # of first eval sentence

        void read_data(std::wistream& is, std::size_t start, std::size_t ns);
    };
}

#endif  // _DATA_CORPUS_HH
