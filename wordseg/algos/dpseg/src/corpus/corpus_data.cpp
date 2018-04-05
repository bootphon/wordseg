#include "corpus/corpus_data.hh"


corpus::corpus_data::corpus_data()
    : corpus_base(), m_evalsent_start(0)
{}


corpus::corpus_data::~corpus_data()
{}


std::vector<sentence> corpus::corpus_data::get_eval_sentences(double init_pboundary, double aeos) const
{
    std::vector<sentence> s;
    std::vector<bool> possible_bs;
    std::vector<bool> true_bs;
    for (uint i = m_evalsent_start; i < m_sentenceboundaries.size()-1; ++i)
    {
        uint start = m_sentenceboundaries[i] - 1;  // include preceding \n
        uint end = m_sentenceboundaries[i + 1];    // this is the ending \n
        initialize_boundaries(start, end, possible_bs, true_bs);
        s.push_back(
            sentence(
                start, end, possible_bs, true_bs,
                nsentences(), init_pboundary, aeos));
    }

    return s;
}


void corpus::corpus_data::read(std::wistream& is, std::size_t start, std::size_t ns)
{
    substring::data.clear();
    m_sentenceboundaries.clear();
    m_true_boundaries.clear();
    m_possible_boundaries.clear();

    substring::data.push_back(L'\n');
    m_true_boundaries.push_back(true);
    m_possible_boundaries.push_back(false);
    m_sentenceboundaries.push_back(substring::data.size());

    // if (debug_level >= 99000) TRACE2(_true_boundaries, _possible_boundaries);
    read_data(is, start, ns);
    // if (debug_level >= 99000) TRACE(sentenceboundaries.size());

    m_ntrainsentences = ns;
    if (m_ntrainsentences == 0)
        m_ntrainsentences = m_sentenceboundaries.size()-1;
    if (m_ntrainsentences >= m_sentenceboundaries.size())
    {
        error("Error: number of training sentences must be less than training data size");
    }

    // note: this means # chars depends on training data only, not
    // eval data.
    initialize_chars();
}


// read additional data for evaluation. This will go into the same
// S::data as the training data.
void corpus::corpus_data::read_eval(std::wistream& is, std::size_t start, std::size_t ns)
{
    m_evalsent_start = m_sentenceboundaries.size()-1;
    read_data(is, start, ns);
}


void corpus::corpus_data::read_data(std::wistream& is, std::size_t start, std::size_t ns)
{
    assert(substring::data.size() >0);
    assert(*(substring::data.end()-1) == L'\n');
    assert(*(m_sentenceboundaries.end()-1) == substring::data.size());

    std::size_t i = m_sentenceboundaries.size()-1;
    std::size_t offset = i;
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
            m_true_boundaries.push_back(true);
            m_possible_boundaries.push_back(true);
        }

        //prev. char was space -- already did boundary info
        else if (m_true_boundaries.size() > substring::data.size())
        {
            if (c == L'\n') error("Input file contains line-final spaces");
            substring::data.push_back(c);
        }
        else
        {
            if (*(substring::data.end()-1) == L'\n' || c == L'\n')
            {
                m_true_boundaries.push_back(true);
                m_possible_boundaries.push_back(false);
            }
            else
            {
                m_true_boundaries.push_back(false);
                m_possible_boundaries.push_back(true);
            }

            substring::data.push_back(c);
            if (c == L'\n')
            {
                m_sentenceboundaries.push_back(substring::data.size());
                i++;
            }
        }

        if (debug_level >= 99000) TRACE3(c, m_true_boundaries, m_possible_boundaries);
    }

    if (*(substring::data.end()-1) != L'\n')
    {
        substring::data.push_back(L'\n');
        m_sentenceboundaries.push_back(substring::data.size());
    }

    if (debug_level >= 98000) TRACE2(substring::data.size(), m_possible_boundaries.size());
    // TRACE2(substring::data.size(), _possible_boundaries.size());

    assert(substring::data.size() >0);
    assert(*(substring::data.end()-1) == L'\n');
    assert(*(m_sentenceboundaries.end()-1) == substring::data.size());
    assert(m_true_boundaries.size() == m_possible_boundaries.size());
    assert(substring::data.size() == m_possible_boundaries.size());
}


void corpus::corpus_data::initialize(std::size_t ns)
{
    m_ntrainsentences = ns;
    if (m_ntrainsentences == 0)
    {
        if (m_evalsent_start > 0)
            m_ntrainsentences = m_evalsent_start;
        else
            m_ntrainsentences = m_sentenceboundaries.size()-1;
    }

    if (m_ntrainsentences >= m_sentenceboundaries.size())
    {
        error("Error: number of training sentences must be less than training data size");
    }

    m_ntrain = m_sentenceboundaries[m_ntrainsentences];
}
