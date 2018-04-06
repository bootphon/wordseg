#include "sentence.hh"


std::wostream& operator<< (std::wostream& os, const substring& s)
{
    return os << s.string();
}


//writes current segmentation
std::wostream& operator<< (std::wostream& os, const sentence& s)
{
    assert(*(s.begin()) == '\n');
    assert(*(s.end()-1) == '\n');
    for (std::size_t i = 1; i < s.boundaries().size()-3; i++)
    {
        os << *(s.begin()+i);
        if (s.boundaries()[i+1])
        {
            os << " ";
        }
    }
    os << *(s.end()-2);
    return os;
}


std::wostream& sentence::print(std::wostream& os) const
{
    assert(*(begin()) == '\n');
    assert(*(end()-1) == '\n');
    os << "chars: $ ";
    for(const_iterator i=begin()+1; i<end()-1; i++)
    {
        os << *i << " ";
    }
    os << "$ |" << std::endl;
    os << "posbs: ";
    std::size_t prev=0;
    for(const auto& i: m_possible_boundaries)
    {
        for (std::size_t j=prev; j<i; j++)
            os << "  ";
        os << "? ";
        prev = i+1;
    }

    for (std::size_t j=prev; j<size(); j++)
        os << "  ";
    os << "| " << m_possible_boundaries << std::endl;
    os << "padbs: ";
    prev=0;

    for(const auto& i: m_padded_possible)
    {
        for (std::size_t j=prev; j<i; j++)
            os << "  ";
        os << "? ";
        prev = i+1;
    }
    for (std::size_t j=prev; j<size(); j++)
        os << "  ";
    os << "| " << m_padded_possible << std::endl;
    os << "bs:    ";

    for(const auto& i: m_boundaries)
    {
        if (i)
            os << ". ";
        else
            os << "  ";
    }
    os << "|" << std::endl;
    os << "true:  ";
    for(const auto& i: m_true_boundaries)
    {
        if (i)
            os << ". ";
        else
            os << "  ";
    }
    os << "|" << std::endl;
    return os;
}


sentence::sentence(std::size_t start, std::size_t end,
                   std::vector<bool>& possible_boundaries,
		   std::vector<bool>& true_boundaries,
                   std::size_t nsentences,
                   double init_pboundary,
                   double aeos)
    : substring(start, end), m_true_boundaries(true_boundaries),
      m_nsentences(nsentences),
      m_init_pboundary(init_pboundary),
      m_aeos(aeos)
{
    m_true_boundaries.push_back(1);
    m_padded_possible.push_back(1);
    for (std::size_t i=0; i<possible_boundaries.size(); i++)
    {
        if (possible_boundaries[i])
        {
            m_possible_boundaries.push_back(i);
            m_padded_possible.push_back(i);
        }
    }
    m_padded_possible.push_back(size() -1);
    std::size_t n = size();
    m_boundaries.resize(n+1);
    std::fill(m_boundaries.begin(), m_boundaries.end(), false);
    m_boundaries[0] = m_boundaries[1] = m_boundaries[n-1] = m_boundaries[n] = true;

    for (std::size_t i = 2; i < n; ++i)
    {
        if (m_init_pboundary == -1)
        {  // initialize with gold boundaries
            m_boundaries[i] = m_true_boundaries[i];
        }
        else
        { //initialize with random boundaries
            if (unif01() < m_init_pboundary)
                m_boundaries[i] = true;
        }
    }
}


sentence::~sentence()
{}


//adds word counts to lexicon
void sentence::insert_words(Unigrams& lex) const
{
    std::size_t i = 1;
    std::size_t j = i+1;
    assert(m_boundaries[i]);
    assert(m_boundaries[m_boundaries.size()-2]);
    while (j < m_boundaries.size()-1)
    {
        if (m_boundaries[i] && m_boundaries[j])
        {
            insert(i,j,lex);
            i=j;
            j=i+1;
        }
        else
        {
            j++;
        }
    }
    if (debug_level >=90000) TRACE(lex);
}


//remove word counts of whole sentence
void sentence::erase_words(Unigrams& lex)
{
    std::size_t i = 1;
    std::size_t j = i+1;
    assert(m_boundaries[i]);
    assert(m_boundaries[m_boundaries.size()-2]);
    while (j < m_boundaries.size()-1)
    {
        if (m_boundaries[i] && m_boundaries[j])
        {
            erase(i,j,lex);
            i=j;
            j=i+1;
        }
        else
        {
            j++;
        }
    }
    if (debug_level >=90000) TRACE(lex);
}


//adds word counts to lexicon
void sentence::insert_words(Bigrams& lex) const
{
    std::size_t k = 0;
    std::size_t i = 1;
    std::size_t j = i+1;
    assert(m_boundaries[k]);
    assert(m_boundaries[i]);
    assert(m_boundaries[m_boundaries.size()-2]);
    assert(m_boundaries[m_boundaries.size()-1]);
    while (j < m_boundaries.size())
    {
        if (m_boundaries[i] && m_boundaries[j])
        {
            insert(k,i,j,lex);
            k=i;
            i=j;
            j=i+1;
        }
        else
        {
            j++;
        }
    }
    if (debug_level >=90000) TRACE(lex);
}


//remove word counts of whole sentence
void sentence::erase_words(Bigrams& lex)
{
    std::size_t k = 0;
    std::size_t i = 1;
    std::size_t j = i+1;
    assert(m_boundaries[k]);
    assert(m_boundaries[i]);
    assert(m_boundaries[m_boundaries.size()-2]);
    assert(m_boundaries[m_boundaries.size()-1]);
    while (j < m_boundaries.size())
    {
        if (m_boundaries[i] && m_boundaries[j])
        {
            erase(k,i,j,lex);
            k=i;
            i=j;
            j=i+1;
        }
        else
        {
            j++;
        }
    }
    if (debug_level >=90000) TRACE(lex);
}


// the sampling method used in our ACL paper.
// results using this function reproduce those of ACL paper, but
// I have not actually checked the probabilities by hand.
std::size_t sentence::sample_by_flips(Unigrams& lex, double temperature)
{
    std::size_t nchanged = 0;
    for(const auto& item: m_possible_boundaries)
    {
        std::size_t i = item;
        std::size_t i0, i1, i2, i3;
        // gets boundaries sufficient for bigram context but only use
        //unigram context
        surrounding_boundaries(i, i0, i1, i2, i3);
        if(m_boundaries[i])
        { // boundary at position i
            erase(i1, i, lex);
            erase(i, i2, lex);
        }
        else
        {  // no boundary at position i
            erase(i1, i2, lex);
        }
        if (debug_level >= 20000) std::wcerr << lex << std::endl;
        double pb = prob_boundary(i1, i, i2, lex, temperature);
        bool newboundary = (pb > unif01());
        if (newboundary)
        {
            insert(i1, i, lex);
            insert(i, i2, lex);
            if (m_boundaries[i] != 1)
                ++nchanged;
            m_boundaries[i] = 1;
        }
        else
        {  // don't insert a boundary at i
            insert(i1, i2, lex);
            if (m_boundaries[i] != 0)
                ++nchanged;
            m_boundaries[i] = 0;
        }
    }
    if (debug_level >= 20000) std::wcerr << lex << std::endl;
    return nchanged;
}

void sentence::sample_one_flip(Unigrams& lex, double temperature, std::size_t boundary_within_sentence)
{
    // used by DecayedMCMC to sample one boundary within a sentence
    // std::cout << "Debug: Made it to sample_one_flip" << std::endl;

    std::size_t i = boundary_within_sentence;
    std::size_t i0, i1, i2, i3;
    // gets boundaries sufficient for bigram context but only use
    // unigram context
    surrounding_boundaries(i, i0, i1, i2, i3);

    // std::cout << "Debug: Found surrounding boundaries" << std::endl;
    if (m_boundaries[i])
    {
        // std::cout << "Debug: Boundary exists, erasing..." << std::endl;
        erase(i1, i, lex);
        erase(i, i2, lex);
        // std::cout << "Debug: erased" << std::endl;
    }
    else
    {  // no boundary at position i
       // std::cout << "Debug: No boundary, erasing..." << std::endl;
       erase(i1, i2, lex);
       // std::cout << "Debug: erased" << std::endl;
    }

    double pb = prob_boundary(i1, i, i2, lex, temperature);
    bool newboundary = (pb > unif01());
    // std::cout << "Debug: calculated probability of boundary" << std::endl;
    if (newboundary)
    {
        // std::cout << "Debug: making new boundary..." << std::endl;
        insert(i1, i, lex);
        insert(i, i2, lex);
        m_boundaries[i] = 1;
        // std::cout << "Debug: boundary and lexemes inserted" << std::endl;
    }
    else
    {  // don't insert a boundary at i
        // std::cout << "Debug: not making a new boundary..." << std::endl;
        insert(i1, i2, lex);
        m_boundaries[i] = 0;
        // std::cout << "Debug: lexemes inserted, boundary set 0" << std::endl;
    }
}


// inference algorithm for bigram sampler from ACL paper
std::size_t sentence::sample_by_flips(Bigrams& lex, double temperature)
{
    std::size_t nchanged = 0;
    for(const auto& item: m_possible_boundaries)
    {
        std::size_t i = item, i0, i1, i2, i3;
        if(debug_level >= 10000) std::wcout << "Sampling boundary " << i << std::endl;
        surrounding_boundaries(i, i0, i1, i2, i3);
        if (m_boundaries[i])
        {
            erase(i0, i1, i, lex);
            erase(i1, i, i2, lex);
            erase(i, i2, i3, lex);
        }
        else
        {  // no m_boundaries at position i
            erase(i0, i1, i2, lex);
            erase(i1, i2, i3, lex);
        }
        double pb = prob_boundary(i0, i1, i, i2, i3, lex, temperature);
        bool newboundary = (pb > unif01());
        if (newboundary)
        {
            insert(i0, i1, i, lex);
            insert(i1, i, i2, lex);
            insert(i, i2, i3, lex);
            if (m_boundaries[i] != 1)
                ++nchanged;
            m_boundaries[i] = 1;
        }
        else
        {  // don't insert a boundary at i
            insert(i0, i1, i2, lex);
            insert(i1, i2, i3, lex);
            if (m_boundaries[i] != 0)
                ++nchanged;
            m_boundaries[i] = 0;
        }
    }

    return nchanged;
}

void sentence::sample_one_flip(Bigrams& lex, double temperature, std::size_t boundary_within_sentence)
{
    // used by DecayedMCMC to sample one boundary within a sentence
    std::size_t i = boundary_within_sentence, i0, i1, i2, i3;
    surrounding_boundaries(i, i0, i1, i2, i3);
    if (m_boundaries[i])
    {
        erase(i0, i1, i, lex);
        erase(i1, i, i2, lex);
        erase(i, i2, i3, lex);
    }
    else
    {  // no m_boundaries at position i
        erase(i0, i1, i2, lex);
        erase(i1, i2, i3, lex);
    }
    double pb = prob_boundary(i0, i1, i, i2, i3, lex, temperature);
    bool newboundary = (pb > unif01());
    if (newboundary)
    {
        insert(i0, i1, i, lex);
        insert(i1, i, i2, lex);
        insert(i, i2, i3, lex);
        m_boundaries[i] = 1;
    }
    else
    {  // don't insert a boundary at i
        insert(i0, i1, i2, lex);
        insert(i1, i2, i3, lex);
        m_boundaries[i] = 0;
    }
}



// implements dynamic program Viterbi segmentation, without
// Metropolis-Hastings correction.
// assumes words from this sentence have already been erased
// from counts in the lexicon if necessary (i.e., for batch).
//
// nsentences is the number of sentences that have been
// seen (preceding this one, if online; except this one,
// if batch).
void sentence::maximize(Unigrams& lex, std::size_t nsentences, double temperature, bool do_mbdp)
{
    // cache some useful constants
    int N_branch = lex.ntokens() - nsentences;
    double p_continue = pow((N_branch +  m_aeos/2.0) /
                            (lex.ntokens() +  m_aeos), 1/temperature);
    if (debug_level >=90000) TRACE(p_continue);

    // create chart for dynamic program
    typedef std::pair<double, std::size_t> Cell; //best prob, index of best prob
    typedef std::vector<Cell> Chart;

    Chart best(m_boundaries.size()-1, Cell(0.0, 0)); //best seg ending at each index
    assert(*(m_padded_possible.begin()) == 1);
    assert(*(m_padded_possible.end()-1) == m_boundaries.size()-2);

    std::vector<std::size_t>::const_iterator i;
    std::vector<std::size_t>::const_iterator j;
    best[1] = Cell(1.0, 0);

    for (j = m_padded_possible.begin() + 1; j <m_padded_possible.end(); j++)
    {
        for (i = m_padded_possible.begin(); i < j; i++)
        {
            double prob;
            if (do_mbdp)
            {
                double mbdp_p = mbdp_prob(lex, word_at(*i, *j), nsentences);
                prob = pow(mbdp_p, 1/temperature) * best[*i].first;
                if (debug_level >=85000) TRACE5(*i,*j,word_at(*i, *j),mbdp_p,prob);
            }
            else
            {
                prob = pow(lex(word_at(*i, *j)), 1 / temperature)
                    * p_continue * best[*i].first;
                if (debug_level >=85000) TRACE5(*i,*j,word_at(*i, *j),lex(word_at(*i, *j)),prob);
            }
            if (prob > best[*j].first)
            {
                best[*j] = Cell(prob, *i);
            }
        }
    }
    if (debug_level >= 70000) TRACE(best);

    // now reconstruct the best segmentation
    std::size_t k;
    for (k = 2; k <m_boundaries.size() -2; k++)
    {
        m_boundaries[k] = false;
    }
    k = m_boundaries.size() -2;
    while (best[k].second >0)
    {
        k = best[k].second;
        m_boundaries[k] = true;
    }
    if (debug_level >= 70000) TRACE(m_boundaries);
}

// implements dynamic program Viterbi segmentation, without
// Metropolis-Hastings correction.
// assumes words from this sentence have already been erased
// from counts in the lexicon if necessary (i.e., for batch).
//
// nsentences is the number of sentences that have been
// seen (preceding this one, if online; except this one,
// if batch).
void sentence::maximize(Bigrams& lex, std::size_t nsentences, double temperature)
{
    // create chart for dynamic program
    typedef std::pair<double, std::size_t> Cell; //best prob, index of best prob
    typedef std::vector<Cell> Row;
    typedef std::vector<Row> Chart;

    // chart stores prob of best seg ending at j going thru i
    Chart best(m_boundaries.size(), Row(m_boundaries.size(), Cell(0.0, 0)));
    assert(*(m_padded_possible.begin()) == 1);
    assert(*(m_padded_possible.end()-1) == m_boundaries.size()-2);

    std::vector<std::size_t>::const_iterator i;
    std::vector<std::size_t>::const_iterator j;
    std::vector<std::size_t>::const_iterator k;
    m_padded_possible.insert(m_padded_possible.begin(), 0); //ugh.

    // initialise base case
    if (debug_level >=100000) TRACE(m_padded_possible);

    for (k = m_padded_possible.begin() + 2; k <m_padded_possible.end(); k++)
    {
        double prob = pow(lex(word_at(0, 1),word_at(1, *k)), 1/temperature);
        if (debug_level >=85000) TRACE3(*k,word_at(0, 1),word_at(1, *k));
        if (debug_level >=85000) TRACE2(lex(word_at(0, 1),word_at(1,*k)),prob);
        best[1][*k] = Cell(prob, 0);
    }

    // dynamic program
    for (k = m_padded_possible.begin() + 3; k <m_padded_possible.end(); k++)
    {
        for (j = m_padded_possible.begin() + 2; j < k; j++)
        {
            for (i = m_padded_possible.begin() + 1; i < j; i++)
            {
                double prob = pow(lex(word_at(*i, *j),word_at(*j, *k)), 1/temperature)
                    * best[*i][*j].first;
                if (debug_level >=85000) TRACE5(*i,*j,*k,word_at(*i, *j),word_at(*j, *k));
                if (debug_level >=85000) TRACE2(lex(word_at(*i, *j),word_at(*j, *k)),prob);
                if (prob > best[*j][*k].first)
                {
                    best[*j][*k] = Cell(prob, *i);
                }
            }
        }
    }

    // final word must be sentence boundary marker.
    std::size_t m = m_boundaries.size() -1;
    j = m_padded_possible.end() -1; // *j is m_boundaries.size() -2
    for (i = m_padded_possible.begin()+1; i <j; i++)
    {
        double prob = pow(lex(word_at(*i, *j),word_at(*j, m)), 1/temperature)
            * best[*i][*j].first;
        if (debug_level >=85000) TRACE5(*i,*j,m,word_at(*i, *j),word_at(*j, m));
        if (debug_level >=85000) TRACE2(lex(word_at(*i, *j),word_at(*j, m)),prob);
        if (prob > best[*j][m].first)
        {
            best[*j][m] = Cell(prob, *i);
        }
    }

    if (debug_level >= 70000) TRACE(best);

    // now reconstruct the best segmentation
    for (m = 2; m <m_boundaries.size() -2; m++)
    {
        m_boundaries[m] = false;
    }
    m = m_boundaries.size() -2;
    std::size_t n = m_boundaries.size() -1;
    while (best[m][n].second >0)
    {
        std::size_t previous = best[m][n].second;
        m_boundaries[previous] = true;
        n = m;
        m = previous;
    }

    m_padded_possible.erase(m_padded_possible.begin()); //ugh.
    if (debug_level >= 70000) TRACE(m_boundaries);
}

// implements dynamic program sampler for full sentence, without
// Metropolis-Hastings correction.
// assumes words from this sentence have already been erased
// from counts in the lexicon if necessary (i.e., for batch).
//
// nsentences is the number of sentences that have been
// seen (preceding this one, if online; except this one,
// if batch).
void sentence::sample_tree(Unigrams& lex, std::size_t nsentences, double temperature, bool do_mbdp)
{
    // cache some useful constants
    int N_branch = lex.ntokens() - nsentences;
    assert(N_branch >= 0);

    double p_continue = pow(
        (N_branch +  m_aeos/2.0) / (lex.ntokens() +  m_aeos), 1/temperature);
    if (debug_level >=90000) TRACE(p_continue);

    // create chart for dynamic program
    typedef std::pair<double, std::size_t>  Transition; // prob, index from whence prob
    typedef std::vector<Transition> Transitions;
    typedef std::pair<double, Transitions> Cell; // total prob, choices
    typedef std::vector<Cell> Chart;

    // chart stores probabilities of different paths up to i
    Chart best(m_boundaries.size()-1);
    assert(*(m_padded_possible.begin()) == 1);
    assert(*(m_padded_possible.end()-1) == m_boundaries.size()-2);

    std::vector<std::size_t>::const_iterator i;
    std::vector<std::size_t>::const_iterator j;
    best[1].first = 1.0;
    for (j = m_padded_possible.begin() + 1; j <m_padded_possible.end(); j++)
    {
        for (i = m_padded_possible.begin(); i < j; i++)
        {
            double prob;
            if (do_mbdp)
            {
                double mbdp_p = mbdp_prob(lex, word_at(*i, *j), nsentences);
                prob = pow(mbdp_p, 1/temperature) * best[*i].first;
                if (debug_level >=85000) TRACE5(*i,*j,word_at(*i, *j),mbdp_p,prob);
            }
            else
            {
                prob = pow(lex(word_at(*i, *j)), 1/temperature)
                    * p_continue * best[*i].first;
                if (debug_level >=85000) TRACE5(*i,*j,word_at(*i, *j),lex(word_at(*i, *j)),prob);
            }
            best[*j].first += prob;
            best[*j].second.push_back(Transition(prob, *i));
        }
    }

    if (debug_level >= 70000) TRACE(best);

    // now sample a segmentation
    std::size_t k;
    for (k = 2; k <m_boundaries.size() -2; k++)
    {
        m_boundaries[k] = false;
    }
    k = m_boundaries.size() -2;
    while (best[k].first < 1.0)
    {
        // sample one of the transitions
        double r = unif01()* best[k].first;
        double total = 0;
        for(const auto& item: best[k].second)
        {
            total += item.first;
            if (r < total)
            {
                k = item.second;
                m_boundaries[k] = true;
                break;
            }
        }
    }

    if (debug_level >= 70000) TRACE(m_boundaries);
}

void sentence::sample_tree(Bigrams& lex, std::size_t nsentences, double temperature)
{
    // create chart for dynamic program
    typedef std::pair<double, std::size_t>  Transition; // prob, index from whence prob
    typedef std::vector<Transition> Transitions;
    typedef std::pair<double, Transitions> Cell; // total prob, choices
    typedef std::vector<Cell> Row;
    typedef std::vector<Row> Chart;

    // chart stores prob of best seg ending at j going thru i
    Chart best(m_boundaries.size(), Row(m_boundaries.size()));
    assert(*(m_padded_possible.begin()) == 1);
    assert(*(m_padded_possible.end()-1) == m_boundaries.size()-2);
    std::vector<std::size_t>::const_iterator i;
    std::vector<std::size_t>::const_iterator j;
    std::vector<std::size_t>::const_iterator k;
    m_padded_possible.insert(m_padded_possible.begin(),0); //ugh.

    // initialise base case
    if (debug_level >=100000) TRACE(m_padded_possible);
    for (k = m_padded_possible.begin() + 2; k <m_padded_possible.end(); k++)
    {
        double prob = pow(lex(word_at(0, 1),word_at(1, *k)), 1/temperature);
        if (debug_level >=85000) TRACE3(*k,word_at(0, 1),word_at(1, *k));
        if (debug_level >=85000) TRACE2(lex(word_at(0, 1),word_at(1,*k)),prob);
        best[1][*k].first += prob;
        best[1][*k].second.push_back(Transition(prob, 0));
    }

    // dynamic program
    for (k = m_padded_possible.begin() + 3; k <m_padded_possible.end(); k++)
    {
        for (j = m_padded_possible.begin() + 2; j < k; j++)
        {
            for (i = m_padded_possible.begin() + 1; i < j; i++)
            {
                double prob = pow(lex(word_at(*i, *j),word_at(*j, *k)), 1/temperature)
                    * best[*i][*j].first;

                if (debug_level >=85000) TRACE5(*i,*j,*k,word_at(*i, *j),word_at(*j, *k));
                if (debug_level >=85000) TRACE2(lex(word_at(*i, *j),word_at(*j, *k)),prob);

                best[*j][*k].first += prob;
                best[*j][*k].second.push_back(Transition(prob, *i));
            }
        }
    }

    // final word must be sentence boundary marker.
    std::size_t m = m_boundaries.size() -1;
    j = m_padded_possible.end() -1; // *j is m_boundaries.size() -2
    for (i = m_padded_possible.begin()+1; i <j; i++)
    {
        double prob = pow(lex(word_at(*i, *j),word_at(*j, m)), 1/temperature)
            * best[*i][*j].first;
        if (debug_level >=85000) TRACE5(*i,*j,m,word_at(*i, *j),word_at(*j, m));
        if (debug_level >=85000) TRACE2(lex(word_at(*i, *j),word_at(*j, m)),prob);
        best[*j][m].first += prob;
        best[*j][m].second.push_back(Transition(prob, *i));
    }
    if (debug_level >= 70000) TRACE(best);

    // now sample a segmentation
    for (m = 2; m <m_boundaries.size() -2; m++)
    {
        m_boundaries[m] = false;
    }
    m = m_boundaries.size() -2;
    std::size_t n = m_boundaries.size() -1;
    while (best[m][n].second[0].second > 0)
    {
        // sample one of the transitions
        double r = unif01()* best[m][n].first;
        double total = 0;
        for(const auto& item: best[m][n].second)
        {
            total += item.first;
            if (r < total)
            {
                std::size_t previous = item.second;
                m_boundaries[previous] = true;
                n = m;
                m = previous;
                break;
            }
        }
    }
    m_padded_possible.erase(m_padded_possible.begin()); //ugh.
    if (debug_level >= 70000) TRACE(m_boundaries);
}


///////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////


substring sentence::word_at(std::size_t left, std::size_t right) const
{
    return substring(left + begin_index(), right + begin_index());
}


std::vector<substring> sentence::get_words(const std::vector<bool>& boundaries) const
{
    if (debug_level >=90000) print(std::wcout);
    std::vector<substring> words;
    std::size_t i = 1;
    std::size_t j = i+1;
    assert(boundaries[i]);
    assert(boundaries[boundaries.size()-2]);
    while (j < boundaries.size()-1) {
        if (boundaries[i] && boundaries[j]) {
            words.push_back(word_at(i,j));
            i=j;
            j=i+1;
        }
        else {
            j++;
        }
    }
    if (debug_level >=90000) TRACE(words);
    return words;
}

//unigram insertion
void sentence::insert(std::size_t left, std::size_t right, Unigrams& lex) const
{
    if (debug_level >= 10000) TRACE3(left, right, word_at(left,right));
    lex.insert(word_at(left, right));
    if (debug_level >= 10000) TRACE("Done");
}

//unigram removal
void sentence::erase(std::size_t left, std::size_t right, Unigrams& lex) const
{
    if (debug_level > 10000) TRACE4("DEBUG ", left, right, word_at(left,right));
    lex.erase(word_at(left,right));
}

// bigram insertion
void sentence::insert(std::size_t i0, std::size_t i1, std::size_t i2, Bigrams& lex) const
{
    if (debug_level >= 20000) TRACE5(i0, i1, i2,word_at(i0,i1), word_at(i1,i2));
    lex.insert(word_at(i0,i1), word_at(i1,i2));
}

// bigram removal
void sentence::erase(std::size_t i0, std::size_t i1, std::size_t i2, Bigrams& lex) const
{
    if (debug_level >= 20000) TRACE5(i0, i1, i2,word_at(i0,i1), word_at(i1,i2));
    lex.erase(word_at(i0,i1), word_at(i1,i2));
}

//unigram probability for flip sampling
// doesn't account for repetitions
double sentence::prob_boundary(
    std::size_t i1, std::size_t i, std::size_t i2, const Unigrams& lex, double temperature) const
{
    if (debug_level >= 100000) TRACE3(i1, i, i2);
    double p_continue = (lex.ntokens() - m_nsentences + 1 + m_aeos/2)/
        (lex.ntokens() + 1 +m_aeos);
    double p_boundary = lex(word_at(i1,i)) * lex(word_at(i,i2)) * p_continue;
    double p_noboundary = lex(word_at(i1,i2));
    if (debug_level >= 50000) TRACE3(p_continue, p_boundary, p_noboundary);
    if (temperature != 1)
    {
        p_boundary = pow(p_boundary, 1/temperature);
        p_noboundary = pow(p_noboundary, 1/temperature);
    }
    assert(finite(p_boundary));
    assert(p_boundary > 0);
    assert(finite(p_noboundary));
    assert(p_noboundary > 0);
    double p = p_boundary/(p_boundary + p_noboundary);
    assert(finite(p));
    return p;
}  // PYEstimator::prob_boundary()

//! p_bigram() is the bigram probability of (i1,i2) following (i0,i1)
double sentence::p_bigram(std::size_t i0, std::size_t i1, std::size_t i2, const Bigrams& lex) const
{
    Bigrams::const_iterator it = lex.find(word_at(i0, i1));
    if (it == lex.end())
        return lex.base_dist()(word_at(i1, i2));
    else
        return it->second(word_at(i1, i2));
}

//! prob_boundary() returns the probability of a boundary at position i
double sentence::prob_boundary(
    std::size_t i0, std::size_t i1, std::size_t i, std::size_t i2, std::size_t i3,
    const Bigrams& lex, double temperature) const
{
    double p_boundary = p_bigram(i0, i1, i, lex) * p_bigram(i1, i, i2, lex) * p_bigram(i, i2, i3, lex);
    double p_noboundary = p_bigram(i0, i1, i2, lex) * p_bigram(i1, i2, i3, lex);
    if (temperature != 1)
    {
        p_boundary = pow(p_boundary, 1/temperature);
        p_noboundary = pow(p_noboundary, 1/temperature);
    }

    assert(finite(p_boundary));
    assert(p_boundary > 0);
    assert(finite(p_noboundary));
    assert(p_noboundary > 0);
    double p = p_boundary/(p_boundary + p_noboundary);
    assert(finite(p));
    return p;
}

//returns the word boundaries surrounding position i
void sentence::surrounding_boundaries(
    std::size_t i, std::size_t& i0, std::size_t& i1, std::size_t& i2, std::size_t& i3) const
{
    std::size_t n = size();
    assert(i > 1);
    assert(i+1 < n);
    i1 = i-1;  //!< boundary preceeding i
    while (!m_boundaries[i1])
        --i1;
    assert(i1 > 0);
    i0 = i1-1;  //!< boundary preceeding i1
    while (!m_boundaries[i0])
        --i0;
    assert(i0 >= 0);
    assert(i0 < i);
    i2 = i+1;   //!< boundary following i
    while (!m_boundaries[i2])
        ++i2;
    assert(i2 < n);
    i3 = i2+1;  //!< boundary following i2
    while (i3 <= n && !m_boundaries[i3])
        ++i3;
    assert(i3 <= n);
}


double sentence::mbdp_prob(Unigrams& lex, const substring& word, std::size_t nsentences) const
{
    // counts include current instance of word, so add one.
    // also include utterance boundaries, incl. one at start.
    double total_tokens = lex.ntokens() + nsentences + 2.0;
    double word_tokens = lex.ntokens(word) + 1.0;
    double prob;
    if (debug_level >=90000) TRACE2(total_tokens, word_tokens);
    if (word_tokens > 1)
    { // have seen this word before
        prob = (word_tokens - 1)/word_tokens;
        prob = prob * prob * word_tokens/total_tokens;
    }
    else
    { // have not seen this word before
        double types = lex.ntypes() + 2.0; //incl. utt boundary and curr wd.
        const P0& base = lex.base_dist();
        const double pi = 4.0*atan(1.0);
        double l_frac = (types - 1)/types;
        double total_base = base(word);

        for(const auto& item: lex.types())
        {
            total_base += base(item.first);
        }

        prob = (6/pi/pi) * (types/total_tokens) * l_frac * l_frac;
        prob *= base(word)/(1 - l_frac*total_base);

        if (debug_level >=87000) TRACE2(base(word), total_base);
        if (debug_level >=90000) TRACE5(pi, types, total_tokens, l_frac, prob);
    }

    return prob;
}
