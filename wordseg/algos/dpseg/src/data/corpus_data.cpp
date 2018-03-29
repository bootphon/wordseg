#include "data/corpus_data.hh"


data::corpus_data::corpus_data() : _evalsent_start(0) {}

data::corpus_data::~corpus_data() {}

Sentences data::corpus_data::get_eval_sentences() const
{
    Sentences s;
    Bs possible_bs;
    Bs true_bs;
    for (U i = _evalsent_start; i < sentenceboundaries.size()-1; ++i)
    {
        U start = sentenceboundaries[i] - 1;  // include preceding \n
        U end = sentenceboundaries[i + 1];    // this is the ending \n
        initialize_boundaries(start, end, possible_bs, true_bs);
        s.push_back(Sentence(start, end, possible_bs, true_bs, this));
    }
    return s;
}

void data::corpus_data::read(std::wistream& is, U start, U ns)
{
    substring::data.clear();
    sentenceboundaries.clear();
    _true_boundaries.clear();
    _possible_boundaries.clear();

    if (debug_level >= 99000) TRACE2(_true_boundaries, _possible_boundaries);

    substring::data.push_back(L'\n');
    _true_boundaries.push_back(true);
    _possible_boundaries.push_back(false);
    sentenceboundaries.push_back(substring::data.size());

    if (debug_level >= 99000) TRACE2(_true_boundaries, _possible_boundaries);

    read_data(is, start, ns);
    if (debug_level >= 99000) TRACE(sentenceboundaries.size());

    ntrainsentences = ns;
    if (ntrainsentences == 0)
        ntrainsentences = sentenceboundaries.size()-1;
    if (ntrainsentences >= sentenceboundaries.size())
    {
        error("Error: number of training sentences must be less than training data size");
    }

    // note: this means # chars depends on training data only, not
    // eval data.
    initialize_chars();
}

// read additional data for evaluation. This will go into the same
// S::data as the training data.
void data::corpus_data::read_eval(std::wistream& is, U start, U ns)
{
    _evalsent_start = sentenceboundaries.size()-1;
    read_data(is, start, ns);
}

void data::corpus_data::read_data(std::wistream& is, U start, U ns)
{
    assert(substring::data.size() >0);
    assert(*(substring::data.end()-1) == L'\n');
    assert(*(sentenceboundaries.end()-1) == substring::data.size());

    U i = sentenceboundaries.size()-1;
    U offset = i;
    wchar_t c;
    while (start > i- offset && is.get(c))
    {
        if (c == L'\n')
            i++;
    }
    i=0;

    //ns == 0 means read all data
    while (is.get(c) && (ns==0 || i < ns))
    {
        if (c == L' ')
        {
            _true_boundaries.push_back(true);
            _possible_boundaries.push_back(true);
        }

        //prev. char was space -- already did boundary info
        else if (_true_boundaries.size() > substring::data.size())
        {
            if (c == L'\n') error("Input file contains line-final spaces");
            substring::data.push_back(c);
        }
        else
        {
            if (*(substring::data.end()-1) == L'\n' || c == L'\n')
            {
                _true_boundaries.push_back(true);
                _possible_boundaries.push_back(false);
            }
            else
            {
                _true_boundaries.push_back(false);
                _possible_boundaries.push_back(true);
            }

            substring::data.push_back(c);
            if (c == L'\n')
            {
                sentenceboundaries.push_back(substring::data.size());
                i++;
            }
        }

        if (debug_level >= 99000) TRACE3(c, _true_boundaries, _possible_boundaries);
        // TRACE3(c, _true_boundaries, _possible_boundaries);
    }

    if (*(substring::data.end()-1) != L'\n')
    {
        substring::data.push_back(L'\n');
        sentenceboundaries.push_back(substring::data.size());
    }

    if (debug_level >= 98000) TRACE2(substring::data.size(), _possible_boundaries.size());
    // TRACE2(substring::data.size(), _possible_boundaries.size());

    assert(substring::data.size() >0);
    assert(*(substring::data.end()-1) == L'\n');
    assert(*(sentenceboundaries.end()-1) == substring::data.size());
    assert(_true_boundaries.size() == _possible_boundaries.size());
    assert(substring::data.size() == _possible_boundaries.size());
}

void data::corpus_data::initialize(U ns)
{
    ntrainsentences = ns;
    if (ntrainsentences == 0)
    {
        if (_evalsent_start > 0)
            ntrainsentences = _evalsent_start;
        else
            ntrainsentences = sentenceboundaries.size()-1;
    }

    if (ntrainsentences >= sentenceboundaries.size())
    {
        error("Error: number of training sentences must be less than training data size");
    }

    ntrain = sentenceboundaries[ntrainsentences];
}
