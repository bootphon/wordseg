#include "estimator/dmcmc.hh"


estimator::dmcmc::dmcmc(double decay_rate, uint samples_per_utt)
    : m_decay_rate(decay_rate), m_samples_per_utt(samples_per_utt)
{}


estimator::dmcmc::~dmcmc()
{}


void estimator::dmcmc::decayed_initialization(std::vector<sentence> _sentences)
{
    // calculate total number of potential boundaries in training set,
    // this is needed for calculating the cumulative decay
    // probabilities cycle through _sentences
    m_num_total_pot_boundaries = 0;
    std::vector<unsigned int> possible_boundaries;
    for(auto& sent: _sentences)
    {
        m_num_total_pot_boundaries += sent.get_possible_boundaries().size();
    }

    if(debug_level >= 1000)
    {
        std::wcout << "total potential boundaries in training corpus "
                   << m_num_total_pot_boundaries << std::endl;
    }

    // initialize _boundaries_num_sampled to be this size
    m_boundaries_num_sampled.resize(m_num_total_pot_boundaries+1);

    // create decay probabilities, uses _decay_rate and
    // _num_total_pot_potboundaries to create binned probability
    // distribution that will be used to find potential boundaries
    // store values in _decay_offset_probs go to one beyond total
    // potential boundaries
    m_decay_offset_probs.resize(m_num_total_pot_boundaries+2);
    for(uint index = 0; index < m_num_total_pot_boundaries + 1; index++)
    {
        // add 1 so that current boundary (index 0) is possible
        m_decay_offset_probs[index] = pow((index+1), (-1) * m_decay_rate);
        if(debug_level >= 10000)
        {
            std::wcout << "_decay_offset_probs[" << index
                  << "] is " << m_decay_offset_probs[index] << std::endl;
        }
    }

    //initialize cumulative decay probability to 0
    m_cum_decay_prob = 0.0;

    //initialize m_num_curr_pot_boundaries to 0
    m_num_curr_pot_boundaries = 0;
}


void estimator::dmcmc::calc_new_cum_prob(sentence& s, std::size_t num_boundaries)
{
    for(std::size_t index = (m_num_curr_pot_boundaries - num_boundaries);
        index < m_num_curr_pot_boundaries; index++)
    {
        m_cum_decay_prob = m_cum_decay_prob + m_decay_offset_probs[index];
        if(debug_level >= 10000)
            std::wcout << "New m_cum_decay_prob, adding offset " << m_decay_offset_probs[index]
                       << ", for index " << index << " is " << m_cum_decay_prob << std::endl;
    }
}

std::size_t estimator::dmcmc::find_boundary_to_sample()
{
    // default: the last boundary
    std::size_t to_sample = m_num_curr_pot_boundaries;

    // find which boundary to sample use probability decay function to
    // determine offset from current m_npotboundaries decay function:
    // prob_sampling_boundary(x) = p, p = (x+1)^(-_decay_rate) [this
    // is done to avoid division errors for 0 offset]. So, given a
    // probability p, we can determine which boundary x by the
    // following:
    //
    // (1) Given the current boundaries seen so far, use
    // decay_offset_probs to sum all the probabilities of the new
    // boundary offsets.  For example, if utterance 1 has 6 boundaries
    // and utterance 2 has 7 boundaries and we're on utterance 2, the
    // previous tot_decay_prob = sum(decay_offset_probs(0..5)); we now
    // want to add the 7 new ones so the new tot_decay_prob =
    // tot_decay_prob + sum(decay_offset_probs(6..12)).  This allows
    // us to not have to keep re-summing decay probs we've already
    // seen. <--- This should be done on an utterance by utterance
    // basis.
    //
    // (2) Generate a random number that = rand (between 0.0 and 1.0)
    // * tot_decay_prob.  This will determine which "bin" (offset)
    // should be selected.
    //
    // (3) To decide which bin, start summing from
    // decay_offset_probs(0).  For each i (starting from 0 and going
    // until i = m_npotboundaries - 1), compare
    // sum(decay_offset_probs(0..i)) and
    // sum(decay_offset_probs(0..i+1)).  If
    // sum(decay_offset_probs(0..i)) <= random number <
    // sum(decay_offset_probs(0..i+1)), then the correct offset is i =
    // 1.  For example, suppose the random number chosen is 1.2, with
    // decay rate 2.  decay_offset_probs[0] = 1, decay_offset_probs[1]
    // = 0.25.  1 <= 1.2 < 1.25, so the offset chosen from the current
    // boundary should be 1.

    double rand_num = unif01() * m_cum_decay_prob;
    if(debug_level >= 10000)
        std::wcout << "random number chosen is " << rand_num << std::endl;

    double curr_cum_sum = 0.0;
    bool found_bin = false;
    std::size_t index = 0;

    // loop through until we find the right bin, summing as we go
    while(!found_bin and (index < m_num_curr_pot_boundaries))
    {
        // compare rand_num with decay_offset_probs[index] and
        // decay_offset_probs[index+1]
        if(debug_level >= 20000)
            std::wcout << "index is " << index << ", comparing " << rand_num << " with " << curr_cum_sum
                       << " and " << (m_decay_offset_probs[index] + curr_cum_sum) << std::endl;

        if((curr_cum_sum <= rand_num) and
           (rand_num < (curr_cum_sum + m_decay_offset_probs[index])))
        {
            // found bin
            found_bin = true;
            to_sample = m_num_curr_pot_boundaries - index;
            if(debug_level >= 20000)
                std::wcout << "found bin: belongs in offset " << index
                           << ", so boundary to sample is " << to_sample << std::endl;
        }

        curr_cum_sum += m_decay_offset_probs[index];
        index++;
    }

    if(!found_bin)
    {
        // belongs in the furthest offset away, at the very beginning
        // of the corpus
        to_sample = 0;

        if(debug_level >= 1000)
            std::wcout << "***Didn't find bin: sampling boundary 0" << std::endl;
    }

    return to_sample;
}

void estimator::dmcmc::find_sent_to_sample(
    std::size_t boundary_to_sample, sentence &s, std::vector<sentence>& sentences_seen)
{
    std::vector<sentence>::iterator si = sentences_seen.end();

    // get to last element
    si--;
    if(debug_level >= 10000)
        std::wcout << "Current sentence looking for boundary "
                   << boundary_to_sample << " in is " << *si << std::endl;

    s = *si;

    // current number boundaries, start with m_num_curr_pot_boundaries
    std::size_t num_boundaries = m_num_curr_pot_boundaries;
    bool sent_found = false;

    // to access boundary x, search from back (most current) to find
    // utterance (most likely due to decay function)
    while(!sent_found)
    {
        // see if is in current utterance ex: to find x = 7, when
        // there are 8 pot boundaries, and sentence has boundaries 4
        // through 8 grab most recent utterance and count how many
        // potential boundaries are there (ex: 5 (4, 5, 6, 7, 8)). Is
        // 7 > (8-5)?  Yes. Search this utterance. Which one?  7-
        // (8-5) = 4th one.
        //this_sent_boundaries = 0;
        std::size_t this_sent_boundaries = s.get_possible_boundaries().size();

        if(debug_level >= 20000)
            std::wcout << "Seeing if boundary " << boundary_to_sample << " is greater than "
                       << num_boundaries << " - " << this_sent_boundaries << std::endl;

        if(boundary_to_sample > (num_boundaries - this_sent_boundaries))
        {
            sent_found = true;
            m_sentence_sampled = si;
            if(debug_level >= 20000)
                std::wcout << "Found sentence containing boundary "
                           << boundary_to_sample << ": " << *si << std::endl;

            m_boundary_within_sentence =
                this_sent_boundaries - (num_boundaries - boundary_to_sample);

            if(debug_level >= 20000)
                std::wcout << "Will be sampling boundary "
                      << m_boundary_within_sentence
                           << " inside this sentence " << std::endl;
        }
        else
        {
            // haven't found it yet - go to next sentence and set up
            // new comparisons
            if(debug_level >= 20000)
                std::wcout << "Haven't found boundary " << boundary_to_sample
                           << " yet, moving on to previous sentence " << std::endl;

            if(si != sentences_seen.begin())
            {
                si--;
                s = *si;
                num_boundaries -= this_sent_boundaries;

                if(debug_level >= 20000)
                    std::wcout << "Now num_boundaries is " << num_boundaries << std::endl;
            }
            else
            {
                if(debug_level >= 1000)
                    std::wcout << "***Couldn't find boundary "
                               << boundary_to_sample
                               << ", so returning first utterance " << std::endl;

                sent_found = true;
                m_sentence_sampled = si;
            }
        }
    }
}

void estimator::dmcmc::replace_sampled_sentence(
    sentence s, std::vector<sentence> &sentences_seen)
{
    std::vector<sentence>::iterator si = sentences_seen.end();
    while((si != m_sentence_sampled) && (si != sentences_seen.begin()))
    {
        si--;
    }

    if(debug_level >= 10000)
        std::wcout << "Replacing this sentence in sentences_seen: " << *si << std::endl
                   << " with this one: " << s << std::endl;
    *si = s;
}



estimator::online_unigram_dmcmc::online_unigram_dmcmc(
const parameters& params, const text::corpus_base& corpus,
    const annealing& anneal, double forget_rate, double decay_rate, std::size_t samples_per_utt)
    : online_unigram(params, corpus, anneal, forget_rate),
      dmcmc(decay_rate, samples_per_utt)
{
    if(debug_level >= 10000)
        std::wcout << "Printing current m_lex:" << std::endl << m_lex << std::endl;

    decayed_initialization(m_sentences);
}


void estimator::online_unigram_dmcmc::estimate_sentence(sentence& s, double temperature)
{
    // update current total potential boundaries
    std::vector<std::size_t> possible_boundaries = s.get_possible_boundaries();
    std::size_t num_boundaries = possible_boundaries.size();

    if(debug_level >= 10000)
    {
        std::wcout << "Number of boundaries in this utterance: " << num_boundaries << std::endl;
    }

    m_num_curr_pot_boundaries += num_boundaries;
    if(debug_level >= 10000)
    {
        std::wcout << "Number of boundaries total: " << m_num_curr_pot_boundaries << std::endl;
    }

    // add current words in sentence to lexicon
    s.insert_words(m_lex);

    for(std::size_t num_samples = 0; num_samples < m_samples_per_utt; num_samples++)
    {
        if(num_samples == 0)
        { // only do this the first time
            calc_new_cum_prob(s, num_boundaries);
        }

        // find boundary to sample
        std::size_t boundary_to_sample = find_boundary_to_sample();

        if(debug_level >= 10000)
            std::wcout << "Boundary to sample is " << boundary_to_sample << std::endl;

        // to keep track of which boundaries have been sampled, use
        // boundary_samples
        m_boundaries_num_sampled[boundary_to_sample]++;

        if(debug_level >= 10000)
            std::wcout << "Updated m_boundaries_num_sampled[" << boundary_to_sample << "] to be "
                       << m_boundaries_num_sampled[boundary_to_sample] << std::endl;

        // locate utterance containing this boundary, which includes
        // figuring out which boundary position j in the utterance
        // dummy utterance to be filled by actual utterance - most
        // often will be curr_utt, though
        sentence sent_to_sample = s;
        find_sent_to_sample(boundary_to_sample, sent_to_sample, m_sentences_seen);

        // sample m_boundary_within_sentence within sent_to_sample use
        // modified form of sample_by_flips
        if(debug_level >= 10000)
            std::wcout << "Sampling this sentence: " << sent_to_sample
                       << " and this boundary " << m_boundary_within_sentence << std::endl;

        // +1 for boundary to account for beginning and end of
        // sentence in lex
        sent_to_sample.sample_one_flip(m_lex, temperature, m_boundary_within_sentence+1);
        if(debug_level >=10000)
            std::wcout << "After sampling this sentence: " << sent_to_sample << std::endl;

        // need to insert updated sentence into m_sentences
        replace_sampled_sentence(sent_to_sample, m_sentences_seen);

        // check that replacement was correct
        if(debug_level >= 10000){
            std::wcout << "After replacement: " << std::endl;
            for(const auto& m_s: m_sentences_seen)
            {
                std::wcout << m_s << std::endl;
            }
        }
    }

    if(debug_level >= 10000)
        std::wcout << "_lex is now: " << std::endl << m_lex << std::endl;
}


estimator::online_bigram_dmcmc::online_bigram_dmcmc(
    const parameters& params, const text::corpus_base& corpus,
    const annealing& anneal, double forget_rate, double decay_rate, std::size_t samples_per_utt)
    : online_bigram(params, corpus, anneal, forget_rate),
      dmcmc(decay_rate, samples_per_utt)
{
    if(debug_level >= 10000)
        std::wcout << "Printing current m_lex:" << std::endl << m_lex << std::endl;

    decayed_initialization(m_sentences);
}


void estimator::online_bigram_dmcmc::estimate_sentence(sentence& s, double temperature)
{
    // update current total potential boundaries
    std::vector<std::size_t> possible_boundaries = s.get_possible_boundaries();
    std::size_t num_boundaries = possible_boundaries.size();
    m_num_curr_pot_boundaries += num_boundaries;

    // add current words in sentence to lexicon
    s.insert_words(m_lex);

    for(std::size_t num_samples = 0; num_samples < m_samples_per_utt; num_samples++)
    {
        // std::wcerr << "dmcmc::online_bigram::estimate_sentence sample=" << num_samples+1 << std::endl;

        if(num_samples == 0)
        { // only do this the first time
            calc_new_cum_prob(s, num_boundaries);
        }

        // find boundary to sample
        std::size_t boundary_to_sample = find_boundary_to_sample();

        // to keep track of which boundaries have been sampled, use boundary_samples
        m_boundaries_num_sampled[boundary_to_sample]++;

        // locate utterance containing this boundary, which includes
        // figuring out which boundary position j in the utterance
        // dummy utterance to be filled by actual utterance - most
        // often will be curr_utt, though
        sentence sent_to_sample = s;
        find_sent_to_sample(boundary_to_sample, sent_to_sample, m_sentences_seen);

        // sample m_boundary_within_sentence within sent_to_sample use
        // modified form of sample_by_flips

        // +1 for boundary to account for beginning and end of sentence in lex
        sent_to_sample.sample_one_flip(m_lex, temperature, m_boundary_within_sentence+1);

        // need to insert updated sentence into m_sentences
        replace_sampled_sentence(sent_to_sample, m_sentences_seen);
    }

    if(debug_level >= 10000)
        std::wcout << "_lex is now: " << std::endl << m_lex << std::endl;
}
