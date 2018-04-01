#include "data/experimental_data.hh"


data::experimental_data::experimental_data()
{}


data::experimental_data::~experimental_data()
{}

void data::experimental_data::read(std::wistream& is, std::size_t start, std::size_t ns)
{
    substring::data.clear();
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
                error("utterance length exceeds maximum specified in Data:: read_mfdata\n");
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
            substring::data.push_back(L'\n');
        }
        else if (utterance.length() > 3 and utterance.substr(0,4) == L"Test")
        {
            training = false;
            testing = true;
            _testboundaries.push_back(substring::data.size());
        }
        else
        {
            if (training)
            {
                for (std::size_t i = 0; i < utterance.size(); i++)
                {
                    substring::data.push_back(utterance[i]);
                }
                substring::data.push_back(L'\n');
                m_sentenceboundaries.push_back(substring::data.size());
            }

            if (testing && !utterance.empty())
            {
                std::size_t breakpt = utterance.find('\t');
                assert(breakpt != utterance.npos);
                for (std::size_t i = 0; i < breakpt; i++)
                {
                    substring::data.push_back(utterance[i]);
                }

                substring::data.push_back('\t');
                _testboundaries.push_back(substring::data.size());
                for (std::size_t i = breakpt+1; i < utterance.size(); i++)
                {
                    substring::data.push_back(utterance[i]);
                }
                substring::data.push_back(L'\n');
                _testboundaries.push_back(substring::data.size());
            }
        }
    }
    if (!testing)
    {
        error("wrong input file format\n");
    }
    else if (*(substring::data.end()-1) != L'\n')
    {
        substring::data.push_back(L'\n');
        _testboundaries.push_back(substring::data.size());
    }
    initialize_chars();
    //  _current_pair = _test_pairs.begin();
}


const std::vector<std::pair<substring, substring> >& data::experimental_data::get_test_pairs() const
{
    return _test_pairs;
}


void data::experimental_data::initialize(std::size_t ns)
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
        if (substring::data[j-1] != L'\n' && substring::data[j] != L'\n')
            m_possible_boundaries[j] = true;

    // _true_boundaries[i] is true iff there really is a boundary at i
    m_true_boundaries.resize(m_ntrain);

    for (std::size_t i = 0; i < m_ntrain; ++i)
        if (substring::data[i] == L'\n' || substring::data[i-1] == L'\n')
            // insert sentence boundaries into _true_boundaries[]
            m_true_boundaries[i] = true;

    assert(_testboundaries.size() % 2 == 1);
    for (std::size_t i = 0; i < _testboundaries.size()-2; i+=2)
    {
        _test_pairs.push_back(
            std::pair<substring, substring>(
                substring(_testboundaries[i],_testboundaries[i+1]-1),
                substring(_testboundaries[i+1],_testboundaries[i+2]-1)));
    }
}
