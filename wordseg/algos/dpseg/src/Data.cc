#include "Data.h"

using namespace std;


inline void error(const char *s)
{
    std::cerr << "error: " << s << std::endl;
    abort();
    exit(1);
}

inline void error(const std::string s)
{
    error(s.c_str());
}

Data::Data() {}

Data::~Data() {}

Sentences Data::get_sentences() const
{
    Sentences s;
    Bs possible_bs;
    Bs true_bs;

    for (U i = 0; i < ntrainsentences; ++i)
    {
        U start = sentenceboundaries[i]-1; // include preceding \n
        U end = sentenceboundaries[i+1];   // this is the ending \n
        initialize_boundaries(start,end,possible_bs,true_bs);
        s.push_back(Sentence(start,end,possible_bs,true_bs,this));
    }

    return s;
}

void Data::initialize_chars()
{
    // may have been set on commandline
    if (! nchartypes)
    {
        std::set<wchar_t> sc;   //!< used to calculate nchartypes
        for (U i = 0; i < S::data.size(); ++i)
            if (S::data[i] != L'\n')
                sc.insert(S::data[i]);
        nchartypes = sc.size();
    }
}

const Us& Data::sentence_boundary_list() const
{
    return sentenceboundaries;
}

const Bs& Data::possible_boundaries() const
{
    return _possible_boundaries;
}

const Bs& Data::true_boundaries() const
{
    return _true_boundaries;
}

U Data::nsentences() const
{
    return ntrainsentences;
}

U Data::nchars() const
{
    return ntrain;
}

std::vector<Sentence> Data::get_eval_sentences() const
{
    return std::vector<Sentence>();
}

void Data::initialize_boundaries(U start, U end, Bs& possible_bs, Bs& true_bs) const
{
    possible_bs.clear();
    true_bs.clear();

    for (U j = start; j < end; j++)
    {
        if (_possible_boundaries[j])
            possible_bs.push_back(true);
        else
            possible_bs.push_back(false);
        if (_true_boundaries[j])
            true_bs.push_back(true);
        else
            true_bs.push_back(false);
    }
}

std::wostream& Data::write_segmented_corpus(
    const Bs& b, std::wostream& os, I begin, I end) const
{
    // negative boundaries mean count from end
    if (begin < 0)
        // plus because begin is negative
        begin = I(sentenceboundaries.size()) + begin - 1;
    if (end <= 0)
        // plus because end is not positive
        end = I(sentenceboundaries.size()) + end - 1;

    assert(begin < end);
    assert(end < I(sentenceboundaries.size()));

    // map to char positions
    begin = sentenceboundaries[begin];
    end = sentenceboundaries[end];

    assert(begin < end);
    assert(end <= I(S::data.size()));
    assert(b.size() >= unsigned(end));
    assert(b[begin] == 1);  // should be a boundary at begin
    assert(b[end-1] == 1);  // and at the end

    for (I i = begin; i < end; ++i)
    {
        if (S::data[i] != L'\n' and S::data[i-1] != L'\n' && b[i])
            os << L' ';
        os << S::data[i];
    }
    return os;
}

//! anneal_temperature() returns the annealing temperature to be used
//! at each iteration.  If anneal_a is zero, we use the annealing
//! schedule from ACL06, where anneal_iterations are broken into 9
//! equal sized bins, where the ith bin has temperature 10/(bin+1).
//! If anneal_a is non-zero, we use a sigmoid based annealing
//! function.
F Data::anneal_temperature(U iteration) const
{
    if (iteration >= anneal_iterations)
        return anneal_stop_temperature;
    if (anneal_a == 0)
    {
        U bin = (9 * iteration) / anneal_iterations + 1;
        F temp = (10.0 / bin - 1) * (anneal_start_temperature - anneal_stop_temperature) / 9.0
            + anneal_stop_temperature;
        return temp;
    }

    F x = F(iteration) / F(anneal_iterations);
    F s = 1 / (1 + exp(anneal_a * (x - anneal_b)));
    F s0 = 1 / (1 + exp(anneal_a * (0 - anneal_b)));
    F s1 = 1 / (1 + exp(anneal_a * (1 - anneal_b)));
    F temp = (anneal_start_temperature - anneal_stop_temperature) *
        (s-s1) / (s0-s1) + anneal_stop_temperature;

    assert(finite(temp));
    return temp;
}

CorpusData::CorpusData() : _evalsent_start(0) {}

CorpusData::~CorpusData() {}

Sentences CorpusData::get_eval_sentences() const
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

void CorpusData::read(std::wistream& is, U start, U ns)
{
    S::data.clear();
    sentenceboundaries.clear();
    _true_boundaries.clear();
    _possible_boundaries.clear();

    if (debug_level >= 99000) TRACE2(_true_boundaries, _possible_boundaries);

    S::data.push_back(L'\n');
    _true_boundaries.push_back(true);
    _possible_boundaries.push_back(false);
    sentenceboundaries.push_back(S::data.size());

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
void CorpusData::read_eval(std::wistream& is, U start, U ns)
{
    _evalsent_start = sentenceboundaries.size()-1;
    read_data(is, start, ns);
}

void CorpusData::read_data(std::wistream& is, U start, U ns)
{
    assert(S::data.size() >0);
    assert(*(S::data.end()-1) == L'\n');
    assert(*(sentenceboundaries.end()-1) == S::data.size());

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
        else if (_true_boundaries.size() > S::data.size())
        {
            if (c == L'\n') error("Input file contains line-final spaces");
            S::data.push_back(c);
        }
        else
        {
            if (*(S::data.end()-1) == L'\n' || c == L'\n')
            {
                _true_boundaries.push_back(true);
                _possible_boundaries.push_back(false);
            }
            else
            {
                _true_boundaries.push_back(false);
                _possible_boundaries.push_back(true);
            }

            S::data.push_back(c);
            if (c == L'\n')
            {
                sentenceboundaries.push_back(S::data.size());
                i++;
            }
        }

        if (debug_level >= 99000) TRACE3(c, _true_boundaries, _possible_boundaries);
        // TRACE3(c, _true_boundaries, _possible_boundaries);
    }

    if (*(S::data.end()-1) != L'\n')
    {
        S::data.push_back(L'\n');
        sentenceboundaries.push_back(S::data.size());
    }

    if (debug_level >= 98000) TRACE2(S::data.size(), _possible_boundaries.size());
    // TRACE2(S::data.size(), _possible_boundaries.size());

    assert(S::data.size() >0);
    assert(*(S::data.end()-1) == L'\n');
    assert(*(sentenceboundaries.end()-1) == S::data.size());
    assert(_true_boundaries.size() == _possible_boundaries.size());
    assert(S::data.size() == _possible_boundaries.size());
}

void CorpusData::initialize(U ns)
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

ExperimentalData::ExperimentalData() {}

ExperimentalData::~ExperimentalData() {}

void ExperimentalData::read(std::wistream& is, U start, U ns)
{
    S::data.clear();
    sentenceboundaries.clear();

    //where in the file are we?
    //bool lexicon = true;
    bool training = false;
    bool testing = false;

    U buffer_max = 1000;
    wchar_t buffer[buffer_max];
    wchar_t c;
    while (is)
    {
        U index = 0;
        // ignore empty lines
        while (is.get(c) && c  == L'\n') {}

        is.putback(c);
        while (is.get(c) && c != L'\n')
        {
            if (index == buffer_max)
            {
                error("utterance length exceeds maximum specified in Data:: read_mfdata\n");
            }
            buffer[index] = c;
            index++;
        }

        buffer[index] = 0;
        wstring utterance(buffer);
        if (utterance.length() > 7 and utterance.substr(0,8) == L"Training")
        {
            //lexicon = false;
            training = true;
            S::data.push_back(L'\n');
        }
        else if (utterance.length() > 3 and utterance.substr(0,4) == L"Test")
        {
            training = false;
            testing = true;
            _testboundaries.push_back(S::data.size());
        }
        else
        {
            if (training)
            {
                for (U i = 0; i < utterance.size(); i++)
                {
                    S::data.push_back(utterance[i]);
                }
                S::data.push_back(L'\n');
                sentenceboundaries.push_back(S::data.size());
            }

            if (testing && !utterance.empty())
            {
                U breakpt = utterance.find('\t');
                assert(breakpt != utterance.npos);
                for (U i = 0; i < breakpt; i++)
                {
                    S::data.push_back(utterance[i]);
                }

                S::data.push_back('\t');
                _testboundaries.push_back(S::data.size());
                for (U i = breakpt+1; i < utterance.size(); i++)
                {
                    S::data.push_back(utterance[i]);
                }
                S::data.push_back(L'\n');
                _testboundaries.push_back(S::data.size());
            }
        }
    }
    if (!testing)
    {
        error("wrong input file format\n");
    }
    else if (*(S::data.end()-1) != L'\n')
    {
        S::data.push_back(L'\n');
        _testboundaries.push_back(S::data.size());
    }
    initialize_chars();
    //  _current_pair = _test_pairs.begin();
}

const TestPairs& ExperimentalData::get_test_pairs() const
{
    return _test_pairs;
}

void ExperimentalData::initialize(U ns)
{
    ntrainsentences = ns;
    if (ntrainsentences == 0)
        ntrainsentences = sentenceboundaries.size()-1;

    if (ntrainsentences >= sentenceboundaries.size())
    {
        error("Error: number of training sentences must be less than training data size\n");
    }
    ntrain = sentenceboundaries[ntrainsentences];

    _possible_boundaries.resize(ntrain,false);

    //assume any non-s boundary can be a word boundary
    for (U j = 2; j <= ntrain; ++j)
        if (S::data[j-1] != L'\n' && S::data[j] != L'\n')
            _possible_boundaries[j] = true;

    // _true_boundaries[i] is true iff there really is a boundary at i
    _true_boundaries.resize(ntrain);

    for (U i = 0; i < ntrain; ++i)
        if (S::data[i] == L'\n' || S::data[i-1] == L'\n')
            // insert sentence boundaries into _true_boundaries[]
            _true_boundaries[i] = true;

    assert(_testboundaries.size() % 2 == 1);
    for (U i = 0; i < _testboundaries.size()-2; i+=2)
    {
        _test_pairs.push_back(
            SS(S(_testboundaries[i],_testboundaries[i+1]-1),
               S(_testboundaries[i+1],_testboundaries[i+2]-1)));
    }
}
