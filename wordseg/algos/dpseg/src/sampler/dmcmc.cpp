#include "sampler/dmcmc.hh"


sampler::dmcmc::dmcmc(F decay_rate, U samples_per_utt)
    : _decay_rate(decay_rate), _samples_per_utt(samples_per_utt)
{}


sampler::dmcmc::~dmcmc()
{}


void sampler::dmcmc::decayed_initialization(Sentences _sentences)
{
    // calculate total number of potential boundaries in training set,
    // this is needed for calculating the cumulative decay
    // probabilities cycle through _sentences
    _num_total_pot_boundaries = 0;
    Us possible_boundaries;
    for(auto& sent: _sentences)
    {
        _num_total_pot_boundaries += sent.get_possible_boundaries().size();
    }

    if(debug_level >= 1000)
    {
        std::wcout << "total potential boundaries in training corpus "
                   << _num_total_pot_boundaries << std::endl;
    }

    // initialize _boundaries_num_sampled to be this size
    _boundaries_num_sampled.resize(_num_total_pot_boundaries+1);

    // create decay probabilities, uses _decay_rate and
    // _num_total_pot_potboundaries to create binned probability
    // distribution that will be used to find potential boundaries
    // store values in _decay_offset_probs go to one beyond total
    // potential boundaries
    _decay_offset_probs.resize(_num_total_pot_boundaries+2);
    for(U index = 0; index < _num_total_pot_boundaries + 1; index++)
    {
        // add 1 so that current boundary (index 0) is possible
        _decay_offset_probs[index] = pow((index+1), (-1)*_decay_rate);
        if(debug_level >= 10000)
        {
            std::wcout << "_decay_offset_probs[" << index
                  << "] is " << _decay_offset_probs[index] << std::endl;
        }
    }

    //initialize cumulative decay probability to 0
    _cum_decay_prob = 0.0;

    //initialize _num_curr_pot_boundaries to 0
    _num_curr_pot_boundaries = 0;
}


void sampler::dmcmc::calc_new_cum_prob(Sentence& s, U num_boundaries)
{
    for(U index = (_num_curr_pot_boundaries - num_boundaries);
        index < _num_curr_pot_boundaries; index++)
    {
        _cum_decay_prob = _cum_decay_prob + _decay_offset_probs[index];
        if(debug_level >= 10000)
            std::wcout << "New _cum_decay_prob, adding offset " << _decay_offset_probs[index]
                       << ", for index " << index << " is " << _cum_decay_prob << std::endl;
    }
}

uint sampler::dmcmc::find_boundary_to_sample()
{
    // default: the last boundary
    U to_sample = _num_curr_pot_boundaries;

    // find which boundary to sample use probability decay function to
    // determine offset from current _npotboundaries decay function:
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
    // until i = _npotboundaries - 1), compare
    // sum(decay_offset_probs(0..i)) and
    // sum(decay_offset_probs(0..i+1)).  If
    // sum(decay_offset_probs(0..i)) <= random number <
    // sum(decay_offset_probs(0..i+1)), then the correct offset is i =
    // 1.  For example, suppose the random number chosen is 1.2, with
    // decay rate 2.  decay_offset_probs[0] = 1, decay_offset_probs[1]
    // = 0.25.  1 <= 1.2 < 1.25, so the offset chosen from the current
    // boundary should be 1.

    F rand_num = unif01() * _cum_decay_prob;
    if(debug_level >= 10000)
        std::wcout << "random number chosen is " << rand_num << std::endl;

    F curr_cum_sum = 0.0;
    bool found_bin = false;
    U index = 0;

    // loop through until we find the right bin, summing as we go
    while(!found_bin and (index < _num_curr_pot_boundaries))
    {
        // compare rand_num with decay_offset_probs[index] and
        // decay_offset_probs[index+1]
        if(debug_level >= 20000)
            std::wcout << "index is " << index << ", comparing " << rand_num << " with " << curr_cum_sum
                       << " and " << (_decay_offset_probs[index] + curr_cum_sum) << std::endl;

        if((curr_cum_sum <= rand_num) and
           (rand_num < (curr_cum_sum + _decay_offset_probs[index])))
        {
            // found bin
            found_bin = true;
            to_sample = _num_curr_pot_boundaries - index;
            if(debug_level >= 20000)
                std::wcout << "found bin: belongs in offset " << index
                           << ", so boundary to sample is " << to_sample << std::endl;
        }

        curr_cum_sum += _decay_offset_probs[index];
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

void sampler::dmcmc::find_sent_to_sample(U boundary_to_sample, Sentence &s, Sentences& sentences_seen)
{
    Sentences::iterator si = sentences_seen.end();

    // get to last element
    si--;
    if(debug_level >= 10000)
        std::wcout << "Current sentence looking for boundary "
                   << boundary_to_sample << " in is " << *si << std::endl;

    s = *si;

    // current number boundaries, start with _num_curr_pot_boundaries
    U num_boundaries = _num_curr_pot_boundaries;
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
        U this_sent_boundaries = s.get_possible_boundaries().size();

        if(debug_level >= 20000)
            std::wcout << "Seeing if boundary " << boundary_to_sample << " is greater than "
                       << num_boundaries << " - " << this_sent_boundaries << std::endl;

        if(boundary_to_sample > (num_boundaries - this_sent_boundaries))
        {
            sent_found = true;
            _sentence_sampled = si;
            if(debug_level >= 20000)
                std::wcout << "Found sentence containing boundary "
                           << boundary_to_sample << ": " << *si << std::endl;

            _boundary_within_sentence =
                this_sent_boundaries - (num_boundaries - boundary_to_sample);

            if(debug_level >= 20000)
                std::wcout << "Will be sampling boundary "
                      << _boundary_within_sentence
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
                _sentence_sampled = si;
            }
        }
    }
}

void sampler::dmcmc::replace_sampled_sentence(Sentence s, Sentences &sentences_seen)
{
    Sentences::iterator si = sentences_seen.end();
    while((si != _sentence_sampled) && (si != sentences_seen.begin()))
    {
        si--;
    }

    if(debug_level >= 10000)
        std::wcout << "Replacing this sentence in sentences_seen: " << *si << std::endl
                   << " with this one: " << s << std::endl;
    *si = s;
}



sampler::online_unigram_dmcmc::online_unigram_dmcmc(
    data::data* constants, F forget_rate, F decay_rate, U samples_per_utt)
    : online_unigram(constants, forget_rate), dmcmc(decay_rate, samples_per_utt)
{
    if(debug_level >= 10000)
        std::wcout << "Printing current _lex:" << std::endl << _lex << std::endl;

    decayed_initialization(_sentences);
}


void sampler::online_unigram_dmcmc::estimate_sentence(Sentence& s, F temperature)
{
    // update current total potential boundaries
    Us possible_boundaries = s.get_possible_boundaries();
    U num_boundaries = possible_boundaries.size();

    if(debug_level >= 10000)
    {
        std::wcout << "Number of boundaries in this utterance: " << num_boundaries << std::endl;
    }

    _num_curr_pot_boundaries += num_boundaries;
    if(debug_level >= 10000)
    {
        std::wcout << "Number of boundaries total: " << _num_curr_pot_boundaries << std::endl;
    }

    // add current words in sentence to lexicon
    s.insert_words(_lex);

    for(U num_samples = 0; num_samples < _samples_per_utt; num_samples++)
    {
        if(num_samples == 0)
        { // only do this the first time
            calc_new_cum_prob(s, num_boundaries);
        }

        // find boundary to sample
        U boundary_to_sample = find_boundary_to_sample();

        if(debug_level >= 10000)
            std::wcout << "Boundary to sample is " << boundary_to_sample << std::endl;

        // to keep track of which boundaries have been sampled, use
        // boundary_samples
        _boundaries_num_sampled[boundary_to_sample]++;

        if(debug_level >= 10000)
            std::wcout << "Updated _boundaries_num_sampled[" << boundary_to_sample << "] to be "
                       << _boundaries_num_sampled[boundary_to_sample] << std::endl;

        // locate utterance containing this boundary, which includes
        // figuring out which boundary position j in the utterance
        // dummy utterance to be filled by actual utterance - most
        // often will be curr_utt, though
        Sentence sent_to_sample = s;
        find_sent_to_sample(boundary_to_sample, sent_to_sample, _sentences_seen);

        // sample _boundary_within_sentence within sent_to_sample use
        // modified form of sample_by_flips
        if(debug_level >= 10000)
            std::wcout << "Sampling this sentence: " << sent_to_sample
                       << " and this boundary " << _boundary_within_sentence << std::endl;

        // +1 for boundary to account for beginning and end of
        // sentence in lex
        sent_to_sample.sample_one_flip(_lex, temperature, _boundary_within_sentence+1);
        if(debug_level >=10000)
            std::wcout << "After sampling this sentence: " << sent_to_sample << std::endl;

        // need to insert updated sentence into _sentences
        replace_sampled_sentence(sent_to_sample, _sentences_seen);

        // check that replacement was correct
        if(debug_level >= 10000){
            std::wcout << "After replacement: " << std::endl;
            for(const auto& _s: _sentences_seen)
            {
                std::wcout << _s << std::endl;
            }
        }
    }

    if(debug_level >= 10000)
        std::wcout << "_lex is now: " << std::endl << _lex << std::endl;
}


sampler::online_bigram_dmcmc::online_bigram_dmcmc(
    data::data* constants, F forget_rate, F decay_rate, U samples_per_utt)
    : online_bigram(constants, forget_rate), dmcmc(decay_rate, samples_per_utt)
{
    if(debug_level >= 10000)
        std::wcout << "Printing current _lex:" << std::endl << _lex << std::endl;

    decayed_initialization(_sentences);
}


void sampler::online_bigram_dmcmc::estimate_sentence(Sentence& s, F temperature)
{
    // update current total potential boundaries
    Us possible_boundaries = s.get_possible_boundaries();
    U num_boundaries = possible_boundaries.size();
    _num_curr_pot_boundaries += num_boundaries;

    // add current words in sentence to lexicon
    s.insert_words(_lex);

    for(U num_samples = 0; num_samples < _samples_per_utt; num_samples++)
    {
        if(num_samples == 0)
        { // only do this the first time
            calc_new_cum_prob(s, num_boundaries);
        }

        // find boundary to sample
        U boundary_to_sample = find_boundary_to_sample();

        // to keep track of which boundaries have been sampled, use boundary_samples
        _boundaries_num_sampled[boundary_to_sample]++;

        // locate utterance containing this boundary, which includes
        // figuring out which boundary position j in the utterance
        // dummy utterance to be filled by actual utterance - most
        // often will be curr_utt, though
        Sentence sent_to_sample = s;
        find_sent_to_sample(boundary_to_sample, sent_to_sample, _sentences_seen);

        // sample _boundary_within_sentence within sent_to_sample use
        // modified form of sample_by_flips

        // +1 for boundary to account for beginning and end of sentence in lex
        sent_to_sample.sample_one_flip(_lex, temperature, _boundary_within_sentence+1);

        // need to insert updated sentence into _sentences
        replace_sampled_sentence(sent_to_sample, _sentences_seen);
    }

    if(debug_level >= 10000)
        std::wcout << "_lex is now: " << std::endl << _lex << std::endl;
}
