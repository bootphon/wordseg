#include "corpus/corpus_experimental.hh"


corpus::corpus_experimental::corpus_experimental()
{}


corpus::corpus_experimental::~corpus_experimental()
{}

void corpus::corpus_experimental::read(std::wistream& is, std::size_t start, std::size_t ns)
{
    std::wstring data;
    m_sentenceboundaries.clear();

    //where in the file are we?
    //bool lexicon = true;
    bool training = false;
    bool testing = false;

    std::size_t buffer_max = 1000;
    wchar_t buffer[buffer_max];
    wchar_t c;
    while (is)
    {
        std::size_t index = 0;
        // ignore empty lines
        while (is.get(c) && c  == L'\n') {}

        is.putback(c);
        while (is.get(c) && c != L'\n')
        {
            if (index == buffer_max)
            {
                error("utterance length exceeds maximum specified in corpus:: read_mfdata\n");
            }
            buffer[index] = c;
            index++;
        }

        buffer[index] = 0;
        std::wstring utterance(buffer);
        if (utterance.length() > 7 and utterance.substr(0,8) == L"Training")
        {
            //lexicon = false;
            training = true;
            data.push_back(L'\n');
        }
        else if (utterance.length() > 3 and utterance.substr(0,4) == L"Test")
        {
            training = false;
            testing = true;
            m_testboundaries.push_back(data.size());
        }
        else
        {
            if (training)
            {
                for (std::size_t i = 0; i < utterance.size(); i++)
                {
                    data.push_back(utterance[i]);
                }
                data.push_back(L'\n');
                m_sentenceboundaries.push_back(data.size());
            }

            if (testing && !utterance.empty())
            {
                std::size_t breakpt = utterance.find('\t');
                assert(breakpt != utterance.npos);
                for (std::size_t i = 0; i < breakpt; i++)
                {
                    data.push_back(utterance[i]);
                }

                data.push_back('\t');
                m_testboundaries.push_back(data.size());
                for (std::size_t i = breakpt+1; i < utterance.size(); i++)
                {
                    data.push_back(utterance[i]);
                }
                data.push_back(L'\n');
                m_testboundaries.push_back(data.size());
            }
        }
    }
    if (!testing)
    {
        error("wrong input file format\n");
    }
    else if (*(data.end()-1) != L'\n')
    {
        data.push_back(L'\n');
        m_testboundaries.push_back(data.size());
    }

    substring::data(data);
    initialize_chars();
    //  _current_pair = _test_pairs.begin();
}


const std::vector<std::pair<substring, substring> >& corpus::corpus_experimental::get_test_pairs() const
{
    return m_test_pairs;
}


void corpus::corpus_experimental::initialize(std::size_t ns)
{
    m_ntrainsentences = ns;
    if (m_ntrainsentences == 0)
        m_ntrainsentences = m_sentenceboundaries.size()-1;

    if (m_ntrainsentences >= m_sentenceboundaries.size())
    {
        error("Error: number of training sentences must be less than training data size\n");
    }
    m_ntrain = m_sentenceboundaries[m_ntrainsentences];

    m_possible_boundaries.resize(m_ntrain,false);

    //assume any non-s boundary can be a word boundary
    for (std::size_t j = 2; j <= m_ntrain; ++j)
        if (substring::data()[j-1] != L'\n' && substring::data()[j] != L'\n')
            m_possible_boundaries[j] = true;

    // _true_boundaries[i] is true iff there really is a boundary at i
    m_true_boundaries.resize(m_ntrain);

    for (std::size_t i = 0; i < m_ntrain; ++i)
        if (substring::data()[i] == L'\n' || substring::data()[i-1] == L'\n')
            // insert sentence boundaries into _true_boundaries[]
            m_true_boundaries[i] = true;

    assert(_testboundaries.size() % 2 == 1);
    for (std::size_t i = 0; i < m_testboundaries.size()-2; i+=2)
    {
        m_test_pairs.push_back(
            std::pair<substring, substring>(
                substring(m_testboundaries[i], m_testboundaries[i+1]-1),
                substring(m_testboundaries[i+1], m_testboundaries[i+2]-1)));
    }
}
