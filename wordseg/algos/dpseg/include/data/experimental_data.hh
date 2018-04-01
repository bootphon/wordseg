#ifndef _DATA_EXPERIMENTAL_H
#define _DATA_EXPERIMENTAL_H

#include "data/data.hh"


namespace data
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
    class experimental_data: public data
    {
    public:
        experimental_data();
        virtual ~experimental_data();

        virtual void read(std::wistream& is, std::size_t start, std::size_t ns);

        void initialize(std::size_t ns);

        const std::vector<std::pair<substring, substring> >& get_test_pairs() const;

    private:
        std::vector<unsigned int> _testboundaries;  // positions of beg/end of test pairs.
        std::vector<std::pair<substring, substring> > _test_pairs;
    };
}

#endif  // _DATA_EXPERIMENTAL_H
