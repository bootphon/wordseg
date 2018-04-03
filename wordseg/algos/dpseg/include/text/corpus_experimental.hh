#ifndef _TEXT_CORPUS_EXPERIMENTAL_H
#define _TEXT_CORPUS_EXPERIMENTAL_H

#include "text/corpus_base.hh"


namespace text
{
    // format of input files from Mike Frank's experimental stimuli:
    // Lexicon: word1<tab>word2<tab>...<tab>wordN
    //
    // Training Sentences:
    // sentence1
    // sentence2
    // ...
    // sentenceM
    //
    // Test Items:
    // test1<tab>distractor1
    // test2<tab>distractor2
    // ...
    // testL<tab>distractorL
    class corpus_experimental: public corpus_base
    {
    public:
        corpus_experimental();
        virtual ~corpus_experimental();

        virtual void read(std::wistream& is, std::size_t start, std::size_t ns);

        void initialize(std::size_t ns);

        const std::vector<std::pair<substring, substring> >& get_test_pairs() const;

    private:
        // positions of beg/end of test pairs
        std::vector<std::size_t> m_testboundaries;

        std::vector<std::pair<substring, substring> > m_test_pairs;
    };
}

#endif  // _TEXT_CORPUS_EXPERIMENTAL_H
