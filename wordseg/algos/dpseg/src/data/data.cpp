#include "data/data.hh"
#include <set>

data::data::data() {}

data::data::~data() {}

Sentences data::data::get_sentences() const
{
    Sentences s;
    std::vector<bool> possible_bs;
    std::vector<bool> true_bs;

    for (uint i = 0; i < ntrainsentences; ++i)
    {
        std::size_t start = sentenceboundaries[i]-1; // include preceding \n
        std::size_t end = sentenceboundaries[i+1];   // this is the ending \n
        initialize_boundaries(start,end,possible_bs,true_bs);
        s.push_back(Sentence(start,end,possible_bs,true_bs, this));
    }

    return s;
}

void data::data::initialize_chars()
{
    // may have been set on commandline
    if (! nchartypes)
    {
        std::set<wchar_t> sc;   //!< used to calculate nchartypes
        for (uint i = 0; i < substring::data.size(); ++i)
            if (substring::data[i] != L'\n')
                sc.insert(substring::data[i]);
        nchartypes = sc.size();
    }
}

const std::vector<unsigned int>& data::data::sentence_boundary_list() const
{
    return sentenceboundaries;
}

const std::vector<bool>& data::data::possible_boundaries() const
{
    return _possible_boundaries;
}

const std::vector<bool>& data::data::true_boundaries() const
{
    return _true_boundaries;
}

uint data::data::nsentences() const
{
    return ntrainsentences;
}

uint data::data::nchars() const
{
    return ntrain;
}

std::vector<Sentence> data::data::get_eval_sentences() const
{
    return std::vector<Sentence>();
}

void data::data::initialize_boundaries(std::size_t start, std::size_t end,
                                       std::vector<bool>& possible_bs, std::vector<bool>& true_bs) const
{
    possible_bs.clear();
    true_bs.clear();

    for (uint j = start; j < end; j++)
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

std::wostream& data::data::write_segmented_corpus(
    const std::vector<bool>& b, std::wostream& os, int begin, int end) const
{
    // negative boundaries mean count from end
    if (begin < 0)
        // plus because begin is negative
        begin = int(sentenceboundaries.size()) + begin - 1;
    if (end <= 0)
        // plus because end is not positive
        end = int(sentenceboundaries.size()) + end - 1;

    assert(begin < end);
    assert(end < int(sentenceboundaries.size()));

    // map to char positions
    begin = sentenceboundaries[begin];
    end = sentenceboundaries[end];

    assert(begin < end);
    assert(end <= int(S::data.size()));
    assert(b.size() >= unsigned(end));
    assert(b[begin] == 1);  // should be a boundary at begin
    assert(b[end-1] == 1);  // and at the end

    for (int i = begin; i < end; ++i)
    {
        if (substring::data[i] != L'\n' and substring::data[i-1] != L'\n' && b[i])
            os << L' ';
        os << substring::data[i];
    }
    return os;
}

//! anneal_temperature() returns the annealing temperature to be used
//! at each iteration.  If anneal_a is zero, we use the annealing
//! schedule from ACL06, where anneal_iterations are broken into 9
//! equal sized bins, where the ith bin has temperature 10/(bin+1).
//! If anneal_a is non-zero, we use a sigmoid based annealing
//! function.
double data::data::anneal_temperature(uint iteration) const
{
    if (iteration >= anneal_iterations)
        return anneal_stop_temperature;
    if (anneal_a == 0)
    {
        uint bin = (9 * iteration) / anneal_iterations + 1;
        double temp = (10.0 / bin - 1) * (anneal_start_temperature - anneal_stop_temperature) / 9.0
            + anneal_stop_temperature;
        return temp;
    }

    double x = double(iteration) / double(anneal_iterations);
    double s = 1 / (1 + exp(anneal_a * (x - anneal_b)));
    double s0 = 1 / (1 + exp(anneal_a * (0 - anneal_b)));
    double s1 = 1 / (1 + exp(anneal_a * (1 - anneal_b)));
    double temp = (anneal_start_temperature - anneal_stop_temperature) *
        (s-s1) / (s0-s1) + anneal_stop_temperature;

    assert(finite(temp));
    return temp;
}
