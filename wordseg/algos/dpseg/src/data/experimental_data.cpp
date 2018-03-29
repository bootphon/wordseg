#include "data/experimental_data.hh"


data::experimental_data::experimental_data()
{}


data::experimental_data::~experimental_data()
{}

void data::experimental_data::read(std::wistream& is, U start, U ns)
{
    substring::data.clear();
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
                for (U i = 0; i < utterance.size(); i++)
                {
                    substring::data.push_back(utterance[i]);
                }
                substring::data.push_back(L'\n');
                sentenceboundaries.push_back(substring::data.size());
            }

            if (testing && !utterance.empty())
            {
                U breakpt = utterance.find('\t');
                assert(breakpt != utterance.npos);
                for (U i = 0; i < breakpt; i++)
                {
                    substring::data.push_back(utterance[i]);
                }

                substring::data.push_back('\t');
                _testboundaries.push_back(substring::data.size());
                for (U i = breakpt+1; i < utterance.size(); i++)
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


const TestPairs& data::experimental_data::get_test_pairs() const
{
    return _test_pairs;
}


void data::experimental_data::initialize(U ns)
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
        if (substring::data[j-1] != L'\n' && substring::data[j] != L'\n')
            _possible_boundaries[j] = true;

    // _true_boundaries[i] is true iff there really is a boundary at i
    _true_boundaries.resize(ntrain);

    for (U i = 0; i < ntrain; ++i)
        if (substring::data[i] == L'\n' || substring::data[i-1] == L'\n')
            // insert sentence boundaries into _true_boundaries[]
            _true_boundaries[i] = true;

    assert(_testboundaries.size() % 2 == 1);
    for (U i = 0; i < _testboundaries.size()-2; i+=2)
    {
        _test_pairs.push_back(
            SS(substring(_testboundaries[i],_testboundaries[i+1]-1),
               substring(_testboundaries[i+1],_testboundaries[i+2]-1)));
    }
}
