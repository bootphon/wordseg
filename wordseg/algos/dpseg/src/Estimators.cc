#include "Estimators.h"

#include <cmath>

using namespace std;


// using the random number generator defined in dpseg.cc
extern uniform01_type<F> unif01;


// returns a random double between 0 and n, inclusive
inline double randd (int n=1)
{
    return n * unif01();
}

// returns 1 random gaussian
inline double rand_normal (double mean=0, double std=1)
{
    double r1 = randd(1);
    double r2 = randd(1);
    return std * std::sqrt(-2 * std::log(r1)) * std::cos(2 * M_PI * r2) + mean;
}

inline double normal_density (double val, double mean=0, double std=1)
{
    return 1.0 / (std * std::sqrt(2 * M_PI))
        * std::exp(-1 * std::pow(val - mean, 2) / (2 * std * std));
}

template<typename T>
inline std::vector<T>& operator+= (std::vector<T>& a, const std::vector<T>& b)
{
    assert(a.size() == b.size());

    for (size_t i=0; i < a.size(); i++)
    {
        a[i] += b[i];
    }
    return a;
}

template<typename T>
inline std::vector<double> operator/ (const std::vector<T>& a, double b)
{
    std::vector<double> c(a.size());
    for (size_t i=0; i < a.size(); i++)
    {
        c[i] = a[i] / b;
    }
    return c;
}


ModelBase::ModelBase(Data* constants):
    _constants(constants),
    _sentences(_constants->get_sentences()),
    _eval_sentences(_constants->get_eval_sentences()),
    _nsentences_seen(_constants->nsentences())
{}

ModelBase::~ModelBase()
{}

//returns the log probability of the current unigram configuration
F ModelBase::log_posterior(const Unigrams& lex) const
{
    F lp1 = lex.base_dist().logprob(); //Word Probs
    if (debug_level >= 110000) TRACE(lp1);
    F tau = _constants->aeos/2.0;
    F ns = _nsentences_seen;
    //sentence length probs: 1st wd of each sent is free.
    F lp2 = lgamma(ns + tau) + lgamma(lex.ntokens()-ns + tau) + lgamma(2*tau)
        - 2*lgamma(tau) - lgamma(lex.ntokens() + 2*tau);
    if (debug_level >= 110000) TRACE(lp2);
    F lp3 = lex.logprob(); //table probs
    if (debug_level >= 110000) TRACE(lp3);
    return lp1 + lp2 + lp3;
}

//returns the log probability of the current bigram configuration
F ModelBase::log_posterior(const Unigrams& ulex, const Bigrams& lex) const
{
    F lp1 = ulex.base_dist().logprob(); // word probabilities
    if (debug_level >= 110000) TRACE(lp1);
    F lp2 = ulex.logprob(); // unigram table probabilities
    if (debug_level >= 110000) TRACE(lp2);
    F lp3 = 0;
    for(const auto& item: lex)
    {
        lp3 += item.second.logprob();  // bigram table probabilities
        if (debug_level >= 125000) TRACE2(item.first, lp3);
    }

    // = lex.logprob(); // table probs
    if (debug_level >= 110000) TRACE(lp3);
    return lp1 + lp2 + lp3;
}

void ModelBase::resample_pyb(Unigrams& lex)
{
    // number of resampling iterations
    U niterations = 20;
    resample_pyb_type pyb_logP(lex, _constants->pyb_gamma_c, _constants->pyb_gamma_s);
    lex.pyb() = slice_sampler1d(
        pyb_logP, lex.pyb(), unif01, 0.0, std::numeric_limits<F>::infinity(),
        0.0, niterations, 100 * niterations);
}

void ModelBase::resample_pya(Unigrams& lex)
{
    // number of resampling iterations
    U niterations = 20;
    resample_pya_type pya_logP(lex, _constants->pya_beta_a, _constants->pya_beta_b);
    lex.pya() = slice_sampler1d(
        pya_logP, lex.pya(), unif01, std::numeric_limits<F>::min(),
        1.0, 0.0, niterations, 100*niterations);
}


/*
//slice sample hyperparameters for unigram model
// NOTE: the slice sampling isn't working yet.
Bs
ModelBase::hypersample(Unigrams& lex, F temp){
Bs changed;
const U nits = 5;  //!< number of alternating samples of pya and pyb
for (U i=0; i<nits; ++i) {
if (lex.pya() > 0) {
resample_pya(lex);
}
if (lex.pyb() > 0) {
resample_pyb(lex);
}
}
changed.push_back(true);
changed.push_back(true);
if (lex.base_dist().p_stop() > 0) {
changed.push_back(sample_hyperparm(lex.base_dist().p_stop(), true, temp));
}
return changed;
}
*/

//sample hyperparameters for unigram model using my MH method
Bs ModelBase::hypersample(Unigrams& lex, F temp)
{
    Bs changed;
    if (lex.pya() > 0)
    {
        changed.push_back(sample_hyperparm(lex.pya(), true, temp));
    }
    else
    {
        changed.push_back(false);
    }

    if (lex.pyb() > 0)
    {
        changed.push_back(sample_hyperparm(lex.pyb(), false, temp));
    }
    else
    {
        changed.push_back(false);
    }

    if (lex.base_dist().p_stop() > 0)
    {
        changed.push_back(sample_hyperparm(lex.base_dist().p_stop(), true, temp));
    }

    return changed;
}

//sample hyperparameters for bigram model
Bs ModelBase::hypersample(Unigrams& ulex, Bigrams& lex, F temp)
{
    Bs changed = hypersample(ulex, temp);
    if (lex.pya() > 0)
    {
        changed.push_back(sample_hyperparm(lex.pya(), true, temp));
    }
    else
    {
        changed.push_back(false);
    }

    if (lex.pyb() > 0)
    {
        changed.push_back(sample_hyperparm(lex.pyb(), false, temp));
    }
    else
    {
        changed.push_back(false);
    }

    return changed;
}

// beta is the hyperparameter to be sampled. Assume beta must be > 0.
// If beta must also be < 1, set flag.  returns true if value of beta
// changed.
bool ModelBase::sample_hyperparm(F& beta, bool is_prob, F temp)
{
    F std_ratio = _constants->hypersampling_ratio;
    if (std_ratio <= 0)
        return false;

    F old_beta = beta;
    F new_beta;
    if (is_prob and old_beta > 0.5)
    {
        new_beta = rand_normal(old_beta, std_ratio*(1-old_beta));
    }
    else
    {
        new_beta = rand_normal(old_beta, std_ratio*old_beta);
    }

    if (new_beta <= 0 || (is_prob && new_beta >= 1))
    {
        TRACE2(new_beta, is_prob);
        error("beta out of range\n");
    }

    F old_p = log_posterior();
    beta = new_beta;
    F new_p = log_posterior();

    F r = exp(new_p-old_p)*
        normal_density(old_beta, new_beta, std_ratio * new_beta) /
        normal_density(new_beta, old_beta, std_ratio * old_beta);
    r = pow(r,1/temp);

    bool changed = false;
    if ((r >= 1) || (r >= randd(1)))
    {
        changed = true;
    }
    else
    {
        beta = old_beta;
    }

    return changed;
}

// make single pass through test data, segmenting based on sampling or
// maximization of each utt, using current counts from training data
// only (i.e. no new counts are added)
void Model::run_eval(std::wostream& os, F temp, bool maximize)
{
    for(auto& sent: _eval_sentences)
    {
        if (debug_level >= 10000) sent.print(std::wcerr);
        estimate_eval_sentence(sent, temp, maximize);
    }

    assert(sanity_check());
}

void UnigramModel::print_statistics(wostream& os, U iter, F temp, bool header)
{
    if (header)
    {
        os << "#Iter"
           << sep << "Temp"
           << sep << "-logP"
           << sep << "a1" << sep << "b1" << sep << "Pstop"
           << std::endl;
    }

    os << iter << sep << temp << sep << -log_posterior() << sep
       << _lex.pya() << sep << _lex.pyb() << sep << _lex.base_dist().p_stop()
       << " ";

    print_scores(os);
}

void UnigramModel::estimate_eval_sentence(Sentence& s, F temperature, bool maximize)
{
    if (maximize)
        s.maximize(
            _lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
    else
        s.sample_tree(
            _lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
}

void BigramModel::print_statistics(wostream& os, U iter, F temp, bool header)
{
    if (header)
    {
        os << "#Iter"
           << sep << "Temp"
           << sep << "-logP"
           << sep << "a1" << sep << "b1" << sep << "Pstop"
           << sep << "a2" << sep << "b2"
           << std::endl;
    }

    os << iter << sep << temp << sep << -log_posterior() << sep
       << _ulex.pya() << sep << _ulex.pyb() << sep << _ulex.base_dist().p_stop() << sep
       << _lex.pya() << sep << _lex.pyb()
       << " ";

    print_scores(os);
}

void BigramModel::estimate_eval_sentence(Sentence& s, F temperature, bool maximize)
{
    if (maximize)
    {
        s.maximize(_lex, _constants->nsentences()-1, temperature);
    }
    else
    {
        s.sample_tree(_lex, _constants->nsentences()-1, temperature);
    }
}


BatchUnigram::BatchUnigram(Data* constants)
    : UnigramModel(constants)
{
    for(const auto& sent: _sentences)
    {
        if (debug_level >= 10000) sent.print(wcerr);
        sent.insert_words(_lex);
    }
    if (debug_level >= 10000) wcerr << _lex << endl;
}

void BatchUnigram::estimate(
    U iters, wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000) wcout << "Inside BatchUnigram estimate " << endl;
    if (_constants->trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    //number of accepts in hyperparm resampling. Init to correct length.
    Bs accepted_anneal(hypersample(1).size());
    Bs accepted(hypersample(1).size());
    U nanneal = 0;
    U n = 0;
    for (U i=1; i <= iters; i++)
    {
        //U nchanged = 0; // if need to print out, use later
        F temperature = _constants->anneal_temperature(i);
        // if (i % 10 == 0) wcerr << ".";
	if (eval_iters && (i % eval_iters == 0))
        {
            os << "Test set after " << i << " iterations of training " << endl;

            // run evaluation over test set
            run_eval(os,temp,maximize);
            print_eval_scores(wcout);
	}

        for(auto& sent: _sentences)
        {
            if (debug_level >= 10000) sent.print(wcerr);
            estimate_sentence(sent, temperature);
            if (debug_level >= 9000) wcerr << _lex << endl;
        }

        if (_constants->hypersampling_ratio)
        {
            if (temperature > 1)
            {
                accepted_anneal += hypersample(temperature);
                nanneal++;
            }
            else
            {
                accepted += hypersample(temperature);
                n++;
            }
        }

        if (_constants->trace_every > 0 and i%_constants->trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }
        assert(sanity_check());
    }

    os << "hyperparm accept rate: ";
    if (_constants->hypersampling_ratio)
        os << accepted_anneal/nanneal << " (during annealing), "
           << accepted/n << " (after)" << endl;
    else
        os << "no hyperparm sampling" << endl;
}


void BatchUnigramViterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.maximize(_lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
    s.insert_words(_lex);
}

void BatchUnigramFlipSampler::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_by_flips(_lex, temperature);
}

void BatchUnigramTreeSampler::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.sample_tree(_lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
    s.insert_words(_lex);
}

DecayedMCMC::DecayedMCMC(F decay_rate, U samples_per_utt)
{
    if(debug_level >= 10000)
        wcout << "Calling DecayedMCMC constructor" << endl;

    _decay_rate = decay_rate;
    _samples_per_utt = samples_per_utt;

    if(debug_level >= 1000)
        wcout << "decay rate is " << _decay_rate
              << " and samples per utt is " << _samples_per_utt << endl;
}

void DecayedMCMC::decayed_initialization(Sentences _sentences)
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
        wcout << "total potential boundaries in training corpus "
              << _num_total_pot_boundaries << endl;
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
            wcout << "_decay_offset_probs[" << index
                  << "] is " << _decay_offset_probs[index] << endl;
        }
    }

    //initialize cumulative decay probability to 0
    _cum_decay_prob = 0.0;

    //initialize _num_curr_pot_boundaries to 0
    _num_curr_pot_boundaries = 0;
}


OnlineUnigramDecayedMCMC::OnlineUnigramDecayedMCMC(
    Data* constants, F forget_rate, F decay_rate, U samples_per_utt)
    : OnlineUnigram(constants, forget_rate), DecayedMCMC(decay_rate, samples_per_utt)
{
    if(debug_level >= 10000)
        wcout << "Printing current _lex:" << endl << _lex << endl;

    decayed_initialization(_sentences);
}

void OnlineUnigram::estimate(
    U iters, wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        wcout << "Inside OnlineUnigram estimate " << endl;

    if (_constants->trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }
    _nsentences_seen = 0;
    for (U i=1; i <= iters; i++)
    {
        F temperature = _constants->anneal_temperature(i);
	if(!is_decayed)
        {
            // if (i % 10 == 0) wcerr << ".";
            if (eval_iters && (i % eval_iters == 0))
            {
                os << "Test set after " << i << " iterations of training " << endl;

                // run evaluation over test set
                run_eval(os,temp,maximize);
                print_eval_scores(wcout);
            }
	}

        for(Sentences::iterator iter = _sentences.begin(); iter != _sentences.end(); ++iter)
        {
            if (debug_level >= 10000) iter->print(wcerr);
            if(!is_decayed)
            {
		forget_items(iter);
            }

            // if (_nsentences_seen % 100 == 0) wcerr << ".";
            if (eval_iters && (_nsentences_seen % eval_iters == 0))
            {
		os << "Test set after " << _nsentences_seen
                   << " sentences of training " << endl;

		// run evaluation over test set
		run_eval(os,temp,maximize);
		print_eval_scores(wcout);
            }

            // add current sentence to _sentences_seen
            _sentences_seen.push_back(*iter);
            estimate_sentence(*iter, temperature);
            _nsentences_seen++;

            if (debug_level >= 9000) TRACE2(-log_posterior(), _lex.ntokens());
            if (debug_level >= 15000)  TRACE2(_lex.ntypes(),_lex);
        }

        if (_constants->trace_every > 0 and i%_constants->trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }
}


void OnlineUnigram::forget_items(Sentences::iterator iter)
{
    if (_forget_rate)
    {
        assert(!_constants->token_memory and !_constants->type_memory);
        if (_sentences.begin() + _forget_rate <= iter)
        {
            if (debug_level >= 9000)
                TRACE(*(iter-_forget_rate));

            (iter - _forget_rate)->erase_words(_lex);
            _nsentences_seen--;
        }
        assert(sanity_check());
    }
    else if (_constants->type_memory)
    {
        while (_lex.ntypes() >_constants->type_memory)
        {
            if (_constants->forget_method == "U")
            {
                _lex.erase_type_uniform();
            }
            else if (_constants->forget_method == "P")
            {
                _lex.erase_type_proportional();
            }
            else
            {
                error("unknown unigram type forget-method");
            }

            if (debug_level >= 15000)
                TRACE2(_lex.ntypes(),_lex);
        }

        // need #sents <= ntoks for math to work.
        if (_lex.ntokens() < _nsentences_seen)
        {
            _nsentences_seen = _lex.ntokens();
        }
        assert(sanity_check());
    }
    else if (_constants->token_memory)
    {
        while (_lex.ntokens() >_constants->token_memory)
        {
            if (_constants->forget_method == "U")
            {
                _lex.erase_token_uniform();
            }
            else
            {
                error("unknown unigram token forget-method");
            }

            if (debug_level >= 15000)  TRACE2(_lex.ntokens(),_lex);
        }

        // need #sents <= ntoks for math to work.
        if (_lex.ntokens() < _nsentences_seen)
        {
            _nsentences_seen = _lex.ntokens();
        }
        assert(sanity_check());
    }
    else if (_constants->token_memory)
    {
        error("unigram forgetting scheme not yet implemented");
    }
}


void DecayedMCMC::calc_new_cum_prob(Sentence& s, U num_boundaries)
{
    for(U index = (_num_curr_pot_boundaries - num_boundaries);
        index < _num_curr_pot_boundaries; index++)
    {
        _cum_decay_prob = _cum_decay_prob + _decay_offset_probs[index];
        if(debug_level >= 10000)
            wcout << "New _cum_decay_prob, adding offset " << _decay_offset_probs[index]
                  << ", for index " << index << " is " << _cum_decay_prob << endl;
    }
}

U DecayedMCMC::find_boundary_to_sample()
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

    F rand_num = randd()*_cum_decay_prob;
    if(debug_level >= 10000)
        wcout << "random number chosen is " << rand_num << endl;

    F curr_cum_sum = 0.0;
    bool found_bin = false;
    U index = 0;

    // loop through until we find the right bin, summing as we go
    while(!found_bin and (index < _num_curr_pot_boundaries))
    {
        // compare rand_num with decay_offset_probs[index] and
        // decay_offset_probs[index+1]
        if(debug_level >= 10000)
            wcout << "index is " << index << ", comparing " << rand_num << " with " << curr_cum_sum
                  << " and " << (_decay_offset_probs[index] + curr_cum_sum) << endl;

        if((curr_cum_sum <= rand_num) and
           (rand_num < (curr_cum_sum + _decay_offset_probs[index])))
        {
            // found bin
            found_bin = true;
            to_sample = _num_curr_pot_boundaries - index;
            if(debug_level >= 10000)
                wcout << "found bin: belongs in offset " << index
                      << ", so boundary to sample is " << to_sample << endl;
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
            wcout << "***Didn't find bin: sampling boundary 0" << endl;
    }

    return to_sample;
}

void DecayedMCMC::find_sent_to_sample(U boundary_to_sample, Sentence &s, Sentences& sentences_seen)
{
    Sentences::iterator si = sentences_seen.end();

    // get to last element
    si--;
    if(debug_level >= 10000)
        wcout << "Current sentence looking for boundary "
              << boundary_to_sample << " in is " << *si << endl;

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

        if(debug_level >= 10000)
            wcout << "Seeing if boundary " << boundary_to_sample << " is greater than "
                  << num_boundaries << " - " << this_sent_boundaries << endl;

        if(boundary_to_sample > (num_boundaries - this_sent_boundaries))
        {
            sent_found = true;
            _sentence_sampled = si;
            if(debug_level >= 10000)
                wcout << "Found sentence containing boundary "
                      << boundary_to_sample << ": " << *si << endl;

            _boundary_within_sentence =
                this_sent_boundaries - (num_boundaries - boundary_to_sample);

            if(debug_level >= 10000)
                wcout << "Will be sampling boundary "
                      << _boundary_within_sentence
                      << " inside this sentence " << endl;
        }
        else
        {
            // haven't found it yet - go to next sentence and set up
            // new comparisons
            if(debug_level >= 10000)
                wcout << "Haven't found boundary " << boundary_to_sample
                      << " yet, moving on to previous sentence " << endl;

            if(si != sentences_seen.begin())
            {
                si--;
                s = *si;
                num_boundaries -= this_sent_boundaries;

                if(debug_level >= 10000)
                    wcout << "Now num_boundaries is " << num_boundaries << endl;
            }
            else
            {
                if(debug_level >= 1000)
                    wcout << "***Couldn't find boundary "
                          << boundary_to_sample
                          << ", so returning first utterance " << endl;

                sent_found = true;
                _sentence_sampled = si;
            }
        }
    }
}

void DecayedMCMC::replace_sampled_sentence(Sentence s, Sentences &sentences_seen)
{
    Sentences::iterator si = sentences_seen.end();
    while((si != _sentence_sampled) && (si != sentences_seen.begin()))
    {
        si--;
    }

    if(debug_level >= 10000)
        wcout << "Replacing this sentence in sentences_seen: " << *si << endl
              << " with this one: " << s << endl;
    *si = s;
}

void OnlineUnigramDecayedMCMC::estimate_sentence(Sentence& s, F temperature)
{
    // update current total potential boundaries
    Us possible_boundaries = s.get_possible_boundaries();
    U num_boundaries = possible_boundaries.size();

    if(debug_level >= 10000)
    {
        wcout << "Number of boundaries in this utterance: " << num_boundaries << endl;
    }

    _num_curr_pot_boundaries += num_boundaries;
    if(debug_level >= 10000)
    {
        wcout << "Number of boundaries total: " << _num_curr_pot_boundaries << endl;
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
            wcout << "Boundary to sample is " << boundary_to_sample << endl;

        // to keep track of which boundaries have been sampled, use
        // boundary_samples
        _boundaries_num_sampled[boundary_to_sample]++;

        if(debug_level >= 10000)
            wcout << "Updated _boundaries_num_sampled[" << boundary_to_sample << "] to be "
                  << _boundaries_num_sampled[boundary_to_sample] << endl;

        // locate utterance containing this boundary, which includes
        // figuring out which boundary position j in the utterance
        // dummy utterance to be filled by actual utterance - most
        // often will be curr_utt, though
        Sentence sent_to_sample = s;
        find_sent_to_sample(boundary_to_sample, sent_to_sample, _sentences_seen);

        // sample _boundary_within_sentence within sent_to_sample use
        // modified form of sample_by_flips
        if(debug_level >= 10000)
            wcout << "Sampling this sentence: " << sent_to_sample
                  << " and this boundary " << _boundary_within_sentence << endl;

        // +1 for boundary to account for beginning and end of
        // sentence in lex
        sent_to_sample.sample_one_flip(_lex, temperature, _boundary_within_sentence+1);
        if(debug_level >=10000)
            wcout << "After sampling this sentence: " << sent_to_sample << endl;

        // need to insert updated sentence into _sentences
        replace_sampled_sentence(sent_to_sample, _sentences_seen);

        // check that replacement was correct
        if(debug_level >= 10000){
            wcout << "After replacement: " << endl;
            for(const auto& _s: _sentences_seen)
            {
                wcout << _s << endl;
            }
        }
    }

    if(debug_level >= 10000)
        wcout << "_lex is now: " << endl << _lex << endl;
}

void OnlineUnigramViterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.maximize(_lex, _nsentences_seen, temperature,_constants->do_mbdp);
    s.insert_words(_lex);
}

void OnlineUnigramTreeSampler::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_tree(_lex, _nsentences_seen, temperature,_constants->do_mbdp);
    s.insert_words(_lex);
}

BatchBigram::BatchBigram(Data* constants)
    : BigramModel(constants)
{
    for(const auto& item: _sentences)
    {
        if (debug_level >= 10000) item.print(wcerr);
        item.insert_words(_lex);
    }

    if (debug_level >= 10000) wcerr << _lex << endl;
}

void BatchBigram::estimate(
    U iters, wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        wcout << "Inside BatchBigram estimate " << endl;

    if (_constants->trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    // number of accepts in hyperparm resampling. Initialize to the
    // correct length
    Bs accepted_anneal(hypersample(1).size());
    Bs accepted(hypersample(1).size());
    U nanneal = 0;
    U n = 0;
    for (U i=1; i <= iters; i++)
    {
        //U nchanged = 0; if need to print out, un-comment
        F temperature = _constants->anneal_temperature(i);

        // if (i % 10 == 0) wcerr << ".";
	if (eval_iters && (i % eval_iters == 0))
        {
            os << "Test set after " << i << " iterations of training " << endl;

            // run evaluation over test set
            run_eval(os,temp,maximize);
            print_eval_scores(wcout);
	}

        for(auto& sent: _sentences)
        {
            if (debug_level >= 10000) sent.print(wcerr);
            estimate_sentence(sent, temperature);
            if (debug_level >= 9000) wcerr << _lex << endl;
        }

        if (_constants->hypersampling_ratio)
        {
            if (temperature > 1)
            {
                accepted_anneal += hypersample(temperature);
                nanneal++;
            }
            else
            {
                accepted += hypersample(temperature);
                n++;
            }
        }

        if (_constants->trace_every > 0 and i%_constants->trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }

    os << "hyperparm accept rate: " << accepted_anneal/nanneal
       << " (during annealing), " << accepted/n << " (after)" << endl;
}

void BatchBigramViterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.maximize(_lex, _constants->nsentences()-1, temperature);
    s.insert_words(_lex);
}

void BatchBigramTreeSampler::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.sample_tree(_lex, _constants->nsentences()-1, temperature);
    s.insert_words(_lex);
}

void BatchBigramFlipSampler::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_by_flips(_lex, temperature);
}

OnlineBigramDecayedMCMC::OnlineBigramDecayedMCMC(
    Data* constants, F forget_rate, F decay_rate, U samples_per_utt)
    : OnlineBigram(constants, forget_rate), DecayedMCMC(decay_rate, samples_per_utt)
{
    if(debug_level >= 10000)
        wcout << "Printing current _lex:" << endl << _lex << endl;

    decayed_initialization(_sentences);
}

void OnlineBigram::estimate(
    U iters, wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        wcout << "Inside OnlineBigram estimate " << endl;

    if (_constants->trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    _nsentences_seen = 0;
    for (U i=1; i <= iters; i++)
    {
        F temperature = _constants->anneal_temperature(i);
	if(!is_decayed)
        {
            // if (i % 10 == 0) wcerr << ".";
            if (eval_iters && (i % eval_iters == 0))
            {
                os << "Test set after " << i << " iterations of training " << endl;

                // run evaluation over test set
                run_eval(os,temp,maximize);
                print_eval_scores(wcout);
            }
	}

        for(auto iter = _sentences.begin(); iter != _sentences.end(); ++iter)
        {
            if (debug_level >= 10000)
                iter->print(wcerr);

            if(!is_decayed)
            {
		forget_items(iter);
            }

            // if (_nsentences_seen % 100 == 0) wcerr << ".";
            if (eval_iters && (_nsentences_seen % eval_iters == 0))
            {
		os << "Test set after " << _nsentences_seen << " sentences of training " << endl;

		// run evaluation over test set
		run_eval(os,temp,maximize);
		print_eval_scores(wcout);
            }

            // add current sentence to _sentences_seen
            _sentences_seen.push_back(*iter);
            estimate_sentence(*iter, temperature);
            _nsentences_seen++;
        }

        if (_constants->trace_every > 0 and i%_constants->trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }
}

void OnlineBigram::forget_items(Sentences::iterator iter)
{
    if (_forget_rate)
    {
        if (_sentences.begin()+_forget_rate <= iter)
        {
            if (debug_level >= 9000)
                TRACE(*(iter-_forget_rate));

            (iter-_forget_rate)->erase_words(_lex);
            _nsentences_seen--;
        }
    }
    else if (_constants->type_memory || _constants->token_memory)
    {
        error("bigram forgetting scheme not yet implemented");
    }
}

void OnlineBigramViterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.maximize(_lex, _nsentences_seen, temperature);
    s.insert_words(_lex);
}

void OnlineBigramTreeSampler::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_tree(_lex, _nsentences_seen, temperature);
    s.insert_words(_lex);
}

void OnlineBigramDecayedMCMC::estimate_sentence(Sentence& s, F temperature)
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
        wcout << "_lex is now: " << endl << _lex << endl;
}
