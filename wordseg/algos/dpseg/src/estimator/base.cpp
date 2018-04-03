#include "estimator/base.hh"
#include "estimator/slice.hpp"


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


estimator::base::base(const parameters& params, const text::corpus_base& corpus, const annealing& anneal):
    m_params(params),
    m_corpus(corpus),
    m_sentences(corpus.get_sentences(m_params.init_pboundary, m_params.aeos)),
    m_eval_sentences(corpus.get_eval_sentences()),
    m_nsentences_seen(corpus.nsentences()),
    m_base_dist(m_params.pstop, corpus.nchartypes()),
    m_annealing(anneal)
{}


estimator::base::~base()
{}


bool estimator::base::sanity_check() const
{
    assert(m_base_dist.nchartypes() == m_corpus.nchartypes());
    assert(m_base_dist.p_stop() < 0 || // if we're learning this parm.
           m_base_dist.p_stop() == m_corpus.Pstop);
    return true;
}


//returns the log probability of the current unigram configuration
double estimator::base::log_posterior(const Unigrams& lex) const
{
    double lp1 = lex.base_dist().logprob(); // word Probs
    if (debug_level >= 110000) TRACE(lp1);

    double tau = m_params.aeos/2.0;
    double ns = m_nsentences_seen;

    // sentence length probs: 1st wd of each sent is free.
    double lp2 = lgamma(ns + tau) + lgamma(lex.ntokens()-ns + tau) + lgamma(2*tau)
        - 2*lgamma(tau) - lgamma(lex.ntokens() + 2*tau);
    if (debug_level >= 110000) TRACE(lp2);

    double lp3 = lex.logprob(); // table probs
    if (debug_level >= 110000) TRACE(lp3);

    return lp1 + lp2 + lp3;
}


// returns the log probability of the current bigram configuration
double estimator::base::log_posterior(const Unigrams& ulex, const Bigrams& lex) const
{
    double lp1 = ulex.base_dist().logprob(); // word probabilities
    if (debug_level >= 110000) TRACE(lp1);

    double lp2 = ulex.logprob(); // unigram table probabilities
    if (debug_level >= 110000) TRACE(lp2);

    double lp3 = 0;
    for(const auto& item: lex)
    {
        lp3 += item.second.logprob();  // bigram table probabilities
        if (debug_level >= 125000) TRACE2(item.first, lp3);
    }
    if (debug_level >= 110000) TRACE(lp3);

    return lp1 + lp2 + lp3;
}


void estimator::base::resample_pyb(Unigrams& lex)
{
    // number of resampling iterations
    uint niterations = 20;
    resample_pyb_type<Unigrams, double> pyb_logP(lex, m_params.pyb_gamma_c, m_params.pyb_gamma_s);
    lex.pyb() = slice_sampler1d(
        pyb_logP, lex.pyb(), unif01, 0.0, std::numeric_limits<double>::infinity(),
        0.0, niterations, 100 * niterations);
}


void estimator::base::resample_pya(Unigrams& lex)
{
    // number of resampling iterations
    uint niterations = 20;
    resample_pya_type<Unigrams, double> pya_logP(lex, m_params.pya_beta_a, m_params.pya_beta_b);
    lex.pya() = slice_sampler1d(
        pya_logP, lex.pya(), unif01, std::numeric_limits<double>::min(),
        1.0, 0.0, niterations, 100*niterations);
}


/*
//slice sample hyperparameters for unigram model
// NOTE: the slice sampling isn't working yet.
Bs
estimator::base::hypersample(Unigrams& lex, double temp){
Bs changed;
const uint nits = 5;  //!< number of alternating samples of pya and pyb
for (uint i=0; i<nits; ++i) {
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
std::vector<bool> estimator::base::hypersample(Unigrams& lex, double temp)
{
    std::vector<bool> changed;
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
std::vector<bool> estimator::base::hypersample(Unigrams& ulex, Bigrams& lex, double temp)
{
    std::vector<bool> changed = hypersample(ulex, temp);
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
bool estimator::base::sample_hyperparm(double& beta, bool is_prob, double temp)
{
    double std_ratio = m_params.hypersampling_ratio;
    if (std_ratio <= 0)
        return false;

    double old_beta = beta;
    double new_beta;
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

    double old_p = log_posterior();
    beta = new_beta;
    double new_p = log_posterior();

    double r = exp(new_p-old_p)*
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


std::vector<double> estimator::base::predict_pairs(
    const std::vector<std::pair<substring, substring> >& test_pairs, const Unigrams& lex) const
{
    std::vector<double> probs;
    for(const auto& tp: test_pairs)
    {
        double p1 = lex(tp.first);
        double p2 = lex(tp.second);
        if (debug_level >= 10000) TRACE2(p1, p2);
        probs.push_back(p1 / (p1 + p2));
    }

    return probs;
}


std::vector<double> estimator::base::predict_pairs(
    const std::vector<std::pair<substring, substring> >& test_pairs, const Bigrams& lex) const
{
    std::vector<double> probs;
    error("estimator::base::predict_pairs is not implemented for bigram models\n");
    return probs;
}


void estimator::base::print_segmented_sentences(std::wostream& os, const std::vector<sentence>& sentences) const
{
    for(const auto& item: sentences)
        os << item << std::endl;
}


void estimator::base::print_scores_sentences(std::wostream& os, const std::vector<sentence>& sentences)
{
    m_scoring.reset();
    for(const auto& item: sentences)
        item.score(m_scoring);
    m_scoring.print_results(os);
}


// make single pass through test data, segmenting based on sampling or
// maximization of each utt, using current counts from training data
// only (i.e. no new counts are added)
void estimator::base::run_eval(std::wostream& os, double temp, bool maximize)
{
    for(auto& sent: m_eval_sentences)
    {
        // if (debug_level >= 10000) sent.print(std::wcerr);
        estimate_eval_sentence(sent, temp, maximize);
    }

    assert(sanity_check());
}


void estimator::base::print_segmented(std::wostream& os) const
{
    print_segmented_sentences(os, m_sentences);
}


void estimator::base::print_eval_segmented(std::wostream& os) const
{
    print_segmented_sentences(os, m_eval_sentences);
}


void estimator::base::print_scores(std::wostream& os)
{
    print_scores_sentences(os, m_sentences);
}

void estimator::base::print_eval_scores(std::wostream& os)
{
    print_scores_sentences(os, m_eval_sentences);
}
