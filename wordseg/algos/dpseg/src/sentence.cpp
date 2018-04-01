#include "sentence.hh"

#include "data/data.hh"
#include "scoring.hh"

using namespace std;

std::wstring substring::data;   //!< global data object, which holds training and eval data

std::wostream& operator<< (std::wostream& os, const substring& s)
{
    return os << s.string();
}

//writes current segmentation
std::wostream& operator<< (std::wostream& os, const Sentence& s)
{
    assert(*(s.begin()) == '\n');
    assert(*(s.end()-1) == '\n');
    for (uint i = 1; i < s._boundaries.size()-3; i++)
    {
        os << *(s.begin()+i);
        if (s._boundaries[i+1])
        {
            os << " ";
        }
    }
    os << *(s.end()-2);
    return os;
}

std::wostream& Sentence::print(std::wostream& os) const
{
    assert(*(begin()) == '\n');
    assert(*(end()-1) == '\n');
    os << "chars: $ ";
    for(const_iterator i=begin()+1; i<end()-1; i++)
    {
        os << *i << " ";
    }
    os << "$ |" << endl;
    os << "posbs: ";
    uint prev=0;
    for(const auto& i: _possible_boundaries)
    {
        for (uint j=prev; j<i; j++)
            os << "  ";
        os << "? ";
        prev = i+1;
    }

    for (uint j=prev; j<size(); j++)
        os << "  ";
    os << "| " << _possible_boundaries << endl;
    os << "padbs: ";
    prev=0;

    for(const auto& i: _padded_possible)
    {
        for (uint j=prev; j<i; j++)
            os << "  ";
        os << "? ";
        prev = i+1;
    }
    for (uint j=prev; j<size(); j++)
        os << "  ";
    os << "| " <<_padded_possible << endl;
    os << "bs:    ";

    for(const auto& i: _boundaries)
    {
        if (i)
            os << ". ";
        else
            os << "  ";
    }
    os << "|" << endl;
    os << "true:  ";
    for(const auto& i: _true_boundaries)
    {
        if (i)
            os << ". ";
        else
            os << "  ";
    }
    os << "|" << endl;
    return os;
}


//initializes possible boundaries and actual boundaries.
Sentence::Sentence(std::size_t start, std::size_t end, std::vector<bool>& possible_boundaries,
		   std::vector<bool>& true_boundaries, const data::data* d)
    : substring(start, end), _true_boundaries(true_boundaries), _constants(d)
{
    _true_boundaries.push_back(1);
    _padded_possible.push_back(1);
    for (uint i=0; i<possible_boundaries.size(); i++)
    {
        if (possible_boundaries[i])
        {
            _possible_boundaries.push_back(i);
            _padded_possible.push_back(i);
        }
    }
    _padded_possible.push_back(size() -1);
    uint n = size();
    _boundaries.resize(n+1);
    std::fill(_boundaries.begin(), _boundaries.end(), false);
    _boundaries[0] = _boundaries[1] = _boundaries[n-1] = _boundaries[n] = true;

    for (uint i = 2; i < n; ++i)
    {
        if (_constants->init_pboundary == -1)
        {  // initialize with gold boundaries
            _boundaries[i] = _true_boundaries[i];
        }
        else
        { //initialize with random boundaries
            if (unif01() < _constants->init_pboundary)
                _boundaries[i] = true;
        }
    }
}


//adds word counts to lexicon
void Sentence::insert_words(Unigrams& lex) const
{
    uint i = 1;
    uint j = i+1;
    assert(_boundaries[i]);
    assert(_boundaries[_boundaries.size()-2]);
    while (j < _boundaries.size()-1)
    {
        if (_boundaries[i] && _boundaries[j])
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
void Sentence::erase_words(Unigrams& lex)
{
    uint i = 1;
    uint j = i+1;
    assert(_boundaries[i]);
    assert(_boundaries[_boundaries.size()-2]);
    while (j < _boundaries.size()-1)
    {
        if (_boundaries[i] && _boundaries[j])
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
void Sentence::insert_words(Bigrams& lex) const
{
    uint k = 0;
    uint i = 1;
    uint j = i+1;
    assert(_boundaries[k]);
    assert(_boundaries[i]);
    assert(_boundaries[_boundaries.size()-2]);
    assert(_boundaries[_boundaries.size()-1]);
    while (j < _boundaries.size())
    {
        if (_boundaries[i] && _boundaries[j])
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
void Sentence::erase_words(Bigrams& lex)
{
    uint k = 0;
    uint i = 1;
    uint j = i+1;
    assert(_boundaries[k]);
    assert(_boundaries[i]);
    assert(_boundaries[_boundaries.size()-2]);
    assert(_boundaries[_boundaries.size()-1]);
    while (j < _boundaries.size())
    {
        if (_boundaries[i] && _boundaries[j])
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
uint Sentence::sample_by_flips(Unigrams& lex, double temperature)
{
    uint nchanged = 0;
    for(const auto& item: _possible_boundaries)
    {
        uint i = item;
        uint i0, i1, i2, i3;
        // gets boundaries sufficient for bigram context but only use
        //unigram context
        surrounding_boundaries(i, i0, i1, i2, i3);
        if(_boundaries[i])
        { // boundary at position i
            erase(i1, i, lex);
            erase(i, i2, lex);
        }
        else
        {  // no boundary at position i
            erase(i1, i2, lex);
        }
        if (debug_level >= 20000) wcerr << lex << endl;
        double pb = prob_boundary(i1, i, i2, lex, temperature);
        bool newboundary = (pb > unif01());
        if (newboundary)
        {
            insert(i1, i, lex);
            insert(i, i2, lex);
            if (_boundaries[i] != 1)
                ++nchanged;
            _boundaries[i] = 1;
        }
        else
        {  // don't insert a boundary at i
            insert(i1, i2, lex);
            if (_boundaries[i] != 0)
                ++nchanged;
            _boundaries[i] = 0;
        }
    }
    if (debug_level >= 20000) wcerr << lex << endl;
    return nchanged;
}

void Sentence::sample_one_flip(Unigrams& lex, double temperature, uint boundary_within_sentence)
{
    // used by DecayedMCMC to sample one boundary within a sentence
    //cout << "Debug: Made it to sample_one_flip\n";
    uint i = boundary_within_sentence;
    uint i0, i1, i2, i3;
    // gets boundaries sufficient for bigram context but only use
    // unigram context
    surrounding_boundaries(i, i0, i1, i2, i3);
    //cout << "Debug: Found surrounding boundaries\n";
    if (_boundaries[i])
    {
        //cout << "Debug: Boundary exists, erasing...\n";
        erase(i1, i, lex);
        erase(i, i2, lex);
        //cout << "Debug: erased\n";
    }
    else
    {  // no boundary at position i
       //cout << "Debug: No boundary, erasing...\n";
        erase(i1, i2, lex);
        //cout << "Debug: erased\n";
    }

    double pb = prob_boundary(i1, i, i2, lex, temperature);
    bool newboundary = (pb > unif01());
    //cout << "Debug: calculated probability of boundary\n";
    if (newboundary)
    {
        //cout << "Debug: making new boundary...\n";
        insert(i1, i, lex);
        insert(i, i2, lex);
        _boundaries[i] = 1;
        //cout << "Debug: boundary and lexemes inserted\n";
    }
    else
    {  // don't insert a boundary at i
        //cout << "Debug: not making a new boundary...\n";
        insert(i1, i2, lex);
        _boundaries[i] = 0;
        //cout << "Debug: lexemes inserted, boundary set 0\n";
    }
}


// inference algorithm for bigram sampler from ACL paper
uint Sentence::sample_by_flips(Bigrams& lex, double temperature)
{
    uint nchanged = 0;
    for(const auto& item: _possible_boundaries)
    {
        uint i = item, i0, i1, i2, i3;
        if(debug_level >= 10000) wcout << "Sampling boundary " << i << endl;
        surrounding_boundaries(i, i0, i1, i2, i3);
        if (_boundaries[i])
        {
            erase(i0, i1, i, lex);
            erase(i1, i, i2, lex);
            erase(i, i2, i3, lex);
        }
        else
        {  // no _boundaries at position i
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
            if (_boundaries[i] != 1)
                ++nchanged;
            _boundaries[i] = 1;
        }
        else
        {  // don't insert a boundary at i
            insert(i0, i1, i2, lex);
            insert(i1, i2, i3, lex);
            if (_boundaries[i] != 0)
                ++nchanged;
            _boundaries[i] = 0;
        }
    }
    return nchanged;
}

void
Sentence::sample_one_flip(Bigrams& lex, double temperature, uint boundary_within_sentence){
    // used by DecayedMCMC to sample one boundary within a sentence
    uint i = boundary_within_sentence, i0, i1, i2, i3;
    surrounding_boundaries(i, i0, i1, i2, i3);
    if (_boundaries[i]) {
        erase(i0, i1, i, lex);
        erase(i1, i, i2, lex);
        erase(i, i2, i3, lex);
    }
    else {  // no _boundaries at position i
        erase(i0, i1, i2, lex);
        erase(i1, i2, i3, lex);
    }
    double pb = prob_boundary(i0, i1, i, i2, i3, lex, temperature);
    bool newboundary = (pb > unif01());
    if (newboundary) {
        insert(i0, i1, i, lex);
        insert(i1, i, i2, lex);
        insert(i, i2, i3, lex);
        _boundaries[i] = 1;
    }
    else {  // don't insert a boundary at i
        insert(i0, i1, i2, lex);
        insert(i1, i2, i3, lex);
        _boundaries[i] = 0;
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
void
Sentence::maximize(Unigrams& lex, uint nsentences, double temperature, bool do_mbdp){
    // cache some useful constants
    int N_branch = lex.ntokens() - nsentences;
    double p_continue = pow((N_branch +  _constants->aeos/2.0) /
                            (lex.ntokens() +  _constants->aeos), 1/temperature);
    if (debug_level >=90000) TRACE(p_continue);
    // create chart for dynamic program
    typedef pair<double, uint> Cell; //best prob, index of best prob
    typedef vector<Cell> Chart;
    Chart best(_boundaries.size()-1, Cell(0.0, 0)); //best seg ending at each index
    assert(*(_padded_possible.begin()) == 1);
    assert(*(_padded_possible.end()-1) == _boundaries.size()-2);
    std::vector<unsigned int>::const_iterator i;
    std::vector<unsigned int>::const_iterator j;
    best[1] = Cell(1.0, 0);
    for (j = _padded_possible.begin() + 1; j <_padded_possible.end(); j++) {
        for (i = _padded_possible.begin(); i < j; i++) {
            double prob;
            if (do_mbdp) {
                double mbdp_p = mbdp_prob(lex, word_at(*i, *j), nsentences);
                prob = pow(mbdp_p, 1/temperature) * best[*i].first;
                if (debug_level >=85000) TRACE5(*i,*j,word_at(*i, *j),mbdp_p,prob);
            }
            else {
                prob = pow(lex(word_at(*i, *j)), 1/temperature)
                    * p_continue * best[*i].first;
                if (debug_level >=85000) TRACE5(*i,*j,word_at(*i, *j),lex(word_at(*i, *j)),prob);
            }
            if (prob > best[*j].first) {
                best[*j] = Cell(prob, *i);
            }
        }
    }
    if (debug_level >= 70000) TRACE(best);
    // now reconstruct the best segmentation
    uint k;
    for (k = 2; k <_boundaries.size() -2; k++) {
        _boundaries[k] = false;
    }
    k = _boundaries.size() -2;
    while (best[k].second >0) {
        k = best[k].second;
        _boundaries[k] = true;
    }
    if (debug_level >= 70000) TRACE(_boundaries);
}

// implements dynamic program Viterbi segmentation, without
// Metropolis-Hastings correction.
// assumes words from this sentence have already been erased
// from counts in the lexicon if necessary (i.e., for batch).
//
// nsentences is the number of sentences that have been
// seen (preceding this one, if online; except this one,
// if batch).
void
Sentence::maximize(Bigrams& lex, uint nsentences, double temperature){
    // create chart for dynamic program
    typedef pair<double, uint> Cell; //best prob, index of best prob
    typedef vector<Cell> Row;
    typedef vector<Row> Chart;
    // chart stores prob of best seg ending at j going thru i
    Chart best(_boundaries.size(), Row(_boundaries.size(), Cell(0.0, 0)));
    assert(*(_padded_possible.begin()) == 1);
    assert(*(_padded_possible.end()-1) == _boundaries.size()-2);
    std::vector<unsigned int>::const_iterator i;
    std::vector<unsigned int>::const_iterator j;
    std::vector<unsigned int>::const_iterator k;
    _padded_possible.insert(_padded_possible.begin(),0); //ugh.
    // initialise base case
    if (debug_level >=100000) TRACE(_padded_possible);
    for (k = _padded_possible.begin() + 2; k <_padded_possible.end(); k++) {
        double prob = pow(lex(word_at(0, 1),word_at(1, *k)), 1/temperature);
        if (debug_level >=85000) TRACE3(*k,word_at(0, 1),word_at(1, *k));
        if (debug_level >=85000) TRACE2(lex(word_at(0, 1),word_at(1,*k)),prob);
        best[1][*k] = Cell(prob, 0);
    }
    // dynamic program
    for (k = _padded_possible.begin() + 3; k <_padded_possible.end(); k++) {
        for (j = _padded_possible.begin() + 2; j < k; j++) {
            for (i = _padded_possible.begin() + 1; i < j; i++) {
                double prob = pow(lex(word_at(*i, *j),word_at(*j, *k)), 1/temperature)
                    * best[*i][*j].first;
                if (debug_level >=85000) TRACE5(*i,*j,*k,word_at(*i, *j),word_at(*j, *k));
                if (debug_level >=85000) TRACE2(lex(word_at(*i, *j),word_at(*j, *k)),prob);
                if (prob > best[*j][*k].first) {
                    best[*j][*k] = Cell(prob, *i);
                }
            }
        }
    }
    // final word must be sentence boundary marker.
    uint m = _boundaries.size() -1;
    j = _padded_possible.end() -1; // *j is _boundaries.size() -2
    for (i = _padded_possible.begin()+1; i <j; i++) {
        double prob = pow(lex(word_at(*i, *j),word_at(*j, m)), 1/temperature)
            * best[*i][*j].first;
        if (debug_level >=85000) TRACE5(*i,*j,m,word_at(*i, *j),word_at(*j, m));
        if (debug_level >=85000) TRACE2(lex(word_at(*i, *j),word_at(*j, m)),prob);
        if (prob > best[*j][m].first) {
            best[*j][m] = Cell(prob, *i);
        }
    }
    if (debug_level >= 70000) TRACE(best);
    // now reconstruct the best segmentation
    for (m = 2; m <_boundaries.size() -2; m++) {
        _boundaries[m] = false;
    }
    m = _boundaries.size() -2;
    uint n = _boundaries.size() -1;
    while (best[m][n].second >0) {
        uint previous = best[m][n].second;
        _boundaries[previous] = true;
        n = m;
        m = previous;
    }
    _padded_possible.erase(_padded_possible.begin()); //ugh.
    if (debug_level >= 70000) TRACE(_boundaries);
}

// implements dynamic program sampler for full sentence, without
// Metropolis-Hastings correction.
// assumes words from this sentence have already been erased
// from counts in the lexicon if necessary (i.e., for batch).
//
// nsentences is the number of sentences that have been
// seen (preceding this one, if online; except this one,
// if batch).
void
Sentence::sample_tree(Unigrams& lex, uint nsentences, double temperature, bool do_mbdp){
    // cache some useful constants
    int N_branch = lex.ntokens() - nsentences;
    assert(N_branch >= 0);
    double p_continue = pow((N_branch +  _constants->aeos/2.0) /
                            (lex.ntokens() +  _constants->aeos), 1/temperature);
    if (debug_level >=90000) TRACE(p_continue);
    // create chart for dynamic program
    typedef pair<double, uint>  Transition; // prob, index from whence prob
    typedef vector<Transition> Transitions;
    typedef pair<double, Transitions> Cell; // total prob, choices
    typedef vector<Cell> Chart;
    // chart stores probabilities of different paths up to i
    Chart best(_boundaries.size()-1);
    assert(*(_padded_possible.begin()) == 1);
    assert(*(_padded_possible.end()-1) == _boundaries.size()-2);
    std::vector<unsigned int>::const_iterator i;
    std::vector<unsigned int>::const_iterator j;
    best[1].first = 1.0;
    for (j = _padded_possible.begin() + 1; j <_padded_possible.end(); j++) {
        for (i = _padded_possible.begin(); i < j; i++) {
            double prob;
            if (do_mbdp) {
                double mbdp_p = mbdp_prob(lex, word_at(*i, *j), nsentences);
                prob = pow(mbdp_p, 1/temperature) * best[*i].first;
                if (debug_level >=85000) TRACE5(*i,*j,word_at(*i, *j),mbdp_p,prob);
            }
            else {
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
    uint k;
    for (k = 2; k <_boundaries.size() -2; k++)
    {
        _boundaries[k] = false;
    }
    k = _boundaries.size() -2;
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
                _boundaries[k] = true;
                break;
            }
        }
    }
    if (debug_level >= 70000) TRACE(_boundaries);
}

void
Sentence::sample_tree(Bigrams& lex, uint nsentences, double temperature) {
    // create chart for dynamic program
    typedef pair<double, uint>  Transition; // prob, index from whence prob
    typedef vector<Transition> Transitions;
    typedef pair<double, Transitions> Cell; // total prob, choices
    typedef vector<Cell> Row;
    typedef vector<Row> Chart;
    // chart stores prob of best seg ending at j going thru i
    Chart best(_boundaries.size(), Row(_boundaries.size()));
    assert(*(_padded_possible.begin()) == 1);
    assert(*(_padded_possible.end()-1) == _boundaries.size()-2);
    std::vector<unsigned int>::const_iterator i;
    std::vector<unsigned int>::const_iterator j;
    std::vector<unsigned int>::const_iterator k;
    _padded_possible.insert(_padded_possible.begin(),0); //ugh.
    // initialise base case
    if (debug_level >=100000) TRACE(_padded_possible);
    for (k = _padded_possible.begin() + 2; k <_padded_possible.end(); k++) {
        double prob = pow(lex(word_at(0, 1),word_at(1, *k)), 1/temperature);
        if (debug_level >=85000) TRACE3(*k,word_at(0, 1),word_at(1, *k));
        if (debug_level >=85000) TRACE2(lex(word_at(0, 1),word_at(1,*k)),prob);
        best[1][*k].first += prob;
        best[1][*k].second.push_back(Transition(prob, 0));
    }
    // dynamic program
    for (k = _padded_possible.begin() + 3; k <_padded_possible.end(); k++) {
        for (j = _padded_possible.begin() + 2; j < k; j++) {
            for (i = _padded_possible.begin() + 1; i < j; i++) {
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
    uint m = _boundaries.size() -1;
    j = _padded_possible.end() -1; // *j is _boundaries.size() -2
    for (i = _padded_possible.begin()+1; i <j; i++) {
        double prob = pow(lex(word_at(*i, *j),word_at(*j, m)), 1/temperature)
            * best[*i][*j].first;
        if (debug_level >=85000) TRACE5(*i,*j,m,word_at(*i, *j),word_at(*j, m));
        if (debug_level >=85000) TRACE2(lex(word_at(*i, *j),word_at(*j, m)),prob);
        best[*j][m].first += prob;
        best[*j][m].second.push_back(Transition(prob, *i));
    }
    if (debug_level >= 70000) TRACE(best);
    // now sample a segmentation
    for (m = 2; m <_boundaries.size() -2; m++) {
        _boundaries[m] = false;
    }
    m = _boundaries.size() -2;
    uint n = _boundaries.size() -1;
    while (best[m][n].second[0].second > 0) {
        // sample one of the transitions
        double r = unif01()* best[m][n].first;
        double total = 0;
        for(const auto& item: best[m][n].second)
        {
            total += item.first;
            if (r < total)
            {
                uint previous = item.second;
                _boundaries[previous] = true;
                n = m;
                m = previous;
                break;
            }
        }
    }
    _padded_possible.erase(_padded_possible.begin()); //ugh.
    if (debug_level >= 70000) TRACE(_boundaries);
}

void
Sentence::score(Scoring& scoring) const {
    scoring._sentences++;
    scoring.add_words_to_lexicon(get_segmented_words(),
                                 scoring._segmented_lex);
    scoring.add_words_to_lexicon(get_reference_words(),
                                 scoring._reference_lex);
    // calculate number of correct words, segmented words,
    // and reference words and add to totals
    const std::vector<bool>& segmented = _boundaries;
    const std::vector<bool>& ref = _true_boundaries;
    assert(segmented.size() == ref.size());
    if (debug_level >=50000) TRACE2(segmented, ref);
    uint s = 2; //start after eos and beginning of 1st word
    uint r = 2;
    bool left_match = 1;
    while (s < segmented.size()-1) {
        if (segmented[s] && ref[r]) {
            scoring._bs_correct++;
            scoring._segmented_bs++;
            scoring._reference_bs++;
            if (left_match) {
                scoring._words_correct++;
            }
            left_match = 1;
            scoring._segmented_words++;
            scoring._reference_words++;
        }
        else if (segmented[s]) {
            scoring._segmented_words++;
            scoring._segmented_bs++;
            left_match = 0;
        }
        else if (ref[r]) {
            scoring._reference_words++;
            scoring._reference_bs++;
            left_match = 0;
        }
        s++;
        r++;
    }
    //subtract right utt boundary
    scoring._bs_correct--;
    scoring._segmented_bs--;
    scoring._reference_bs--;
    if (debug_level >=60000) TRACE3(scoring._bs_correct, scoring._segmented_bs, scoring._reference_bs);
    if (debug_level >=60000) TRACE3(scoring._words_correct, scoring._segmented_words, scoring._reference_words);
}

///////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////

Sentence::Words Sentence::get_words(const std::vector<bool>& boundaries) const {
    if (debug_level >=90000) print(wcout);
    Words words;
    uint i = 1;
    uint j = i+1;
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
void Sentence::insert(uint left, uint right, Unigrams& lex) const {
    if (debug_level >= 20000) TRACE3(left, right, word_at(left,right));
    lex.insert(word_at(left,right));
}

//unigram removal
void Sentence::erase(uint left, uint right, Unigrams& lex) const {
    if (debug_level >= 20000) TRACE3(left, right, word_at(left,right));
    lex.erase(word_at(left,right));
}

// bigram insertion
void Sentence::insert(uint i0, uint i1, uint i2, Bigrams& lex) const {
    if (debug_level >= 20000) TRACE5(i0, i1, i2,word_at(i0,i1), word_at(i1,i2));
    lex.insert(word_at(i0,i1), word_at(i1,i2));
}

// bigram removal
void Sentence::erase(uint i0, uint i1, uint i2, Bigrams& lex) const {
    if (debug_level >= 20000) TRACE5(i0, i1, i2,word_at(i0,i1), word_at(i1,i2));
    lex.erase(word_at(i0,i1), word_at(i1,i2));
} // PYEstimator::erase()

//unigram probability for flip sampling
// doesn't account for repetitions
double Sentence::prob_boundary(uint i1, uint i, uint i2, const Unigrams& lex, double temperature) const {
    if (debug_level >= 100000) TRACE3(i1, i, i2);
    double p_continue = (lex.ntokens() - _constants->nsentences() + 1 + _constants->aeos/2)/
        (lex.ntokens() + 1 +_constants->aeos);
    double p_boundary = lex(word_at(i1,i)) * lex(word_at(i,i2)) * p_continue;
    double p_noboundary = lex(word_at(i1,i2));
    if (debug_level >= 50000) TRACE3(p_continue, p_boundary, p_noboundary);
    if (temperature != 1) {
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
double Sentence::p_bigram(uint i0, uint i1, uint i2, const Bigrams& lex) const {
    Bigrams::const_iterator it = lex.find(word_at(i0, i1));
    if (it == lex.end())
        return lex.base_dist()(word_at(i1, i2));
    else
        return it->second(word_at(i1, i2));
}

//! prob_boundary() returns the probability of a boundary at position i
double Sentence::prob_boundary(uint i0, uint i1, uint i, uint i2, uint i3, const Bigrams& lex, double temperature) const {
    double p_boundary = p_bigram(i0, i1, i, lex) * p_bigram(i1, i, i2, lex) * p_bigram(i, i2, i3, lex);
    double p_noboundary = p_bigram(i0, i1, i2, lex) * p_bigram(i1, i2, i3, lex);
    if (temperature != 1) {
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
void Sentence::surrounding_boundaries(uint i, uint& i0, uint& i1, uint& i2, uint& i3) const {
    uint n = size();
    assert(i > 1);
    assert(i+1 < n);
    i1 = i-1;  //!< boundary preceeding i
    while (!_boundaries[i1])
        --i1;
    assert(i1 > 0);
    i0 = i1-1;  //!< boundary preceeding i1
    while (!_boundaries[i0])
        --i0;
    assert(i0 >= 0);
    assert(i0 < i);
    i2 = i+1;   //!< boundary following i
    while (!_boundaries[i2])
        ++i2;
    assert(i2 < n);
    i3 = i2+1;  //!< boundary following i2
    while (i3 <= n && !_boundaries[i3])
        ++i3;
    assert(i3 <= n);
}

double Sentence::mbdp_prob(Unigrams& lex, const substring& word, uint nsentences) const
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
        Unigrams::WordTypes items = lex.types();
        for(const auto& item: items)
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
