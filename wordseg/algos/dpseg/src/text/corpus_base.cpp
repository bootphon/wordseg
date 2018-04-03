#include "text/corpus_base.hh"
#include <set>


text::corpus_base::corpus_base()
    : m_sentenceboundaries(),
      m_possible_boundaries(),
      m_true_boundaries(),
      m_nchartypes(),
      m_ntrainsentences(),
      m_ntrain()
{}


text::corpus_base::~corpus_base()
{}


std::vector<sentence> text::corpus_base::get_sentences(double init_pboundary, double aeos) const
{
    std::vector<sentence> s;
    std::vector<bool> possible_bs;
    std::vector<bool> true_bs;

    for (uint i = 0; i < m_ntrainsentences; ++i)
    {
        std::size_t start = m_sentenceboundaries[i] - 1; // include preceding \n
        std::size_t end = m_sentenceboundaries[i + 1];   // this is the ending \n
        initialize_boundaries(start, end, possible_bs, true_bs);
        s.push_back(
            sentence(
                start, end, possible_bs, true_bs,
                nsentences(), init_pboundary, aeos));
    }

    return s;
}

void text::corpus_base::initialize_chars()
{
    // may have been set on commandline
    if (! m_nchartypes)
    {
        std::set<wchar_t> sc;   //!< used to calculate nchartypes
        for (uint i = 0; i < substring::data.size(); ++i)
            if (substring::data[i] != L'\n')
                sc.insert(substring::data[i]);
        m_nchartypes = sc.size();
    }
}

void text::corpus_base::initialize_boundaries(
    std::size_t start, std::size_t end,
    std::vector<bool>& possible_bs, std::vector<bool>& true_bs) const
{
    possible_bs.clear();
    true_bs.clear();

    for (std::size_t j = start; j < end; j++)
    {
        if (m_possible_boundaries[j])
            possible_bs.push_back(true);
        else
            possible_bs.push_back(false);
        if (m_true_boundaries[j])
            true_bs.push_back(true);
        else
            true_bs.push_back(false);
    }
}

const std::vector<std::size_t>& text::corpus_base::sentence_boundary_list() const
{
    return m_sentenceboundaries;
}

const std::vector<bool>& text::corpus_base::possible_boundaries() const
{
    return m_possible_boundaries;
}

const std::vector<bool>& text::corpus_base::true_boundaries() const
{
    return m_true_boundaries;
}

std::size_t text::corpus_base::nchartypes() const
{
    return m_nchartypes;
}

std::size_t text::corpus_base::nsentences() const
{
    return m_ntrainsentences;
}

std::size_t text::corpus_base::nchars() const
{
    return m_ntrain;
}

std::vector<sentence> text::corpus_base::get_eval_sentences() const
{
    return std::vector<sentence>();
}

std::wostream& text::corpus_base::write_segmented_corpus(
    std::wostream& os, const std::vector<bool>& b, int begin, int end) const
{
    // negative boundaries mean count from end
    if (begin < 0)
        // plus because begin is negative
        begin = int(m_sentenceboundaries.size()) + begin - 1;
    if (end <= 0)
        // plus because end is not positive
        end = int(m_sentenceboundaries.size()) + end - 1;

    assert(begin < end);
    assert(end < int(sentenceboundaries.size()));

    // map to char positions
    begin = m_sentenceboundaries[begin];
    end = m_sentenceboundaries[end];

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
