#include "sampler/base.hh"
#include "slice_sampler.hpp"


// using the random number generator defined in dpseg.cc
extern uniform01_type unif01;


// returns 1 random gaussian
inline double rand_normal(double mean=0, double std=1)
{
    double r1 = unif01();
    double r2 = unif01();
    return std * std::sqrt(-2 * std::log(r1)) * std::cos(2 * M_PI * r2) + mean;
}


inline double normal_density (double val, double mean=0, double std=1)
{
    return 1.0 / (std * std::sqrt(2 * M_PI))
        * std::exp(-1 * std::pow(val - mean, 2) / (2 * std * std));
}


sampler::base::base(const data::data& constants):
    _constants(constants),
    _sentences(constants.get_sentences()),
    _eval_sentences(constants.get_eval_sentences()),
    _nsentences_seen(constants.nsentences()),
    _base_dist(constants.Pstop, constants.nchartypes)
{}


sampler::base::~base()
{}


bool sampler::base::sanity_check() const
{
    assert(_base_dist.nchartypes() ==_constants.nchartypes);
    assert(_base_dist.p_stop() < 0 || // if we're learning this parm.
           _base_dist.p_stop() ==_constants.Pstop);
    return true;
}


//returns the log probability of the current unigram configuration
F sampler::base::log_posterior(const Unigrams& lex) const
{
    F lp1 = lex.base_dist().logprob(); // word Probs
    if (debug_level >= 110000) TRACE(lp1);

    F tau = _constants.aeos/2.0;
    F ns = _nsentences_seen;

    // sentence length probs: 1st wd of each sent is free.
    F lp2 = lgamma(ns + tau) + lgamma(lex.ntokens()-ns + tau) + lgamma(2*tau)
        - 2*lgamma(tau) - lgamma(lex.ntokens() + 2*tau);
    if (debug_level >= 110000) TRACE(lp2);

    F lp3 = lex.logprob(); // table probs
    if (debug_level >= 110000) TRACE(lp3);

    return lp1 + lp2 + lp3;
}


// returns the log probability of the current bigram configuration
F sampler::base::log_posterior(const Unigrams& ulex, const Bigrams& lex) const
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
    if (debug_level >= 110000) TRACE(lp3);

    return lp1 + lp2 + lp3;
}


void sampler::base::resample_pyb(Unigrams& lex)
{
    // number of resampling iterations
    U niterations = 20;
    resample_pyb_type pyb_logP(lex, _constants.pyb_gamma_c, _constants.pyb_gamma_s);
    lex.pyb() = slice_sampler1d(
        pyb_logP, lex.pyb(), unif01, 0.0, std::numeric_limits<F>::infinity(),
        0.0, niterations, 100 * niterations);
}


void sampler::base::resample_pya(Unigrams& lex)
{
    // number of resampling iterations
    U niterations = 20;
    resample_pya_type pya_logP(lex, _constants.pya_beta_a, _constants.pya_beta_b);
    lex.pya() = slice_sampler1d(
        pya_logP, lex.pya(), unif01, std::numeric_limits<F>::min(),
        1.0, 0.0, niterations, 100*niterations);
}


/*
//slice sample hyperparameters for unigram model
// NOTE: the slice sampling isn't working yet.
Bs
sampler::base::hypersample(Unigrams& lex, F temp){
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
Bs sampler::base::hypersample(Unigrams& lex, F temp)
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
Bs sampler::base::hypersample(Unigrams& ulex, Bigrams& lex, F temp)
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
bool sampler::base::sample_hyperparm(F& beta, bool is_prob, F temp)
{
    F std_ratio = _constants.hypersampling_ratio;
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
    if ((r >= 1) || (r >= unif01()))
    {
        changed = true;
    }
    else
    {
        beta = old_beta;
    }

    return changed;
}


Fs sampler::base::predict_pairs(const TestPairs& test_pairs, const Unigrams& lex) const
{
    Fs probs;
    for(const auto& tp: test_pairs)
    {
        F p1 = lex(tp.first);
        F p2 = lex(tp.second);
        if (debug_level >= 10000) TRACE2(p1, p2);
        probs.push_back(p1 / (p1 + p2));
    }

    return probs;
}


Fs sampler::base::predict_pairs(const TestPairs& test_pairs, const Bigrams& lex) const
{
    Fs probs;
    error("sampler::base::predict_pairs is not implemented for bigram models\n");
    return probs;
}


void sampler::base::print_segmented_sentences(std::wostream& os, const Sentences& sentences) const
{
    for(const auto& item: sentences)
        os << item << std::endl;
}


void sampler::base::print_scores_sentences(std::wostream& os, const Sentences& sentences)
{
    _scoring.reset();
    for(const auto& item: sentences)
        item.score(_scoring);
    _scoring.print_results(os);
}


// make single pass through test data, segmenting based on sampling or
// maximization of each utt, using current counts from training data
// only (i.e. no new counts are added)
void sampler::base::run_eval(std::wostream& os, F temp, bool maximize)
{
    for(auto& sent: _eval_sentences)
    {
        if (debug_level >= 10000) sent.print(std::wcerr);
        estimate_eval_sentence(sent, temp, maximize);
    }

    assert(sanity_check());
}


void sampler::base::print_segmented(std::wostream& os) const
{
    print_segmented_sentences(os, _sentences);
}


void sampler::base::print_eval_segmented(std::wostream& os) const
{
    print_segmented_sentences(os, _eval_sentences);
}


void sampler::base::print_scores(std::wostream& os)
{
    print_scores_sentences(os, _sentences);
}

void sampler::base::print_eval_scores(std::wostream& os)
{
    print_scores_sentences(os, _eval_sentences);
}
