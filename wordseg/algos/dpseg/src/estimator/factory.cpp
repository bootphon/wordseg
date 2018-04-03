#include <iostream>

#include "estimator/factory.hh"
#include "estimator/dmcmc.hh"
#include "estimator/flip.hh"
#include "estimator/tree.hh"
#include "estimator/viterbi.hh"


std::shared_ptr<estimator::base> estimator::get_estimator(
    const estimator::parameters& params,
    const text::corpus_data& corpus,
    const annealing& anneal,
    const std::size_t ngram,
    const std::string& mode,
    const std::string& estimator,
    const double forget_rate,
    const double decay_rate,
    const std::size_t samples_per_utt)
{
    std::cerr << "Init sampler with ngram=" << ngram << ", " << mode << ", " << estimator << std::endl;

    using model_ptr = std::shared_ptr<estimator::base>;
    model_ptr sampler;

    // make sure the ngram is 1 or 2
    if(ngram != 1 and ngram != 2)
    {
        std::cerr << "Error: ngram must be 1 or 2, it is " << ngram << std::endl;
        return NULL;
    }

    // make sure the mode is "batch" or "online"
    if(mode != "batch" and mode != "online")
    {
        std::cerr << "Error: invalid mode must be batch or online, it is " << mode << std::endl;
        return NULL;
    }

    // make sure the estimator is valid
    std::vector<std::string> estimators = {"F", "V", "T", "D"};
    if(std::find(estimators.begin(), estimators.end(), estimator) == estimators.end())
    {
        std::cerr << "Error: " << estimator << " is not a valid estimator" << std::endl;
        return NULL;
    }

    // bigram batch sampler
    if(ngram == 2 and mode == "batch")
    {
        if(estimator == "F")
        {
            // TRACE(data.nsentences());
            // TRACE(corpus.get_sentences());
            // TRACE(corpus.get_eval_sentences());
            // TRACE(corpus.nchartypes());

            sampler = model_ptr(new estimator::batch_bigram_flip(params, corpus, anneal));
        }
        else if(estimator == "V")
        {
            sampler = model_ptr(new estimator::batch_bigram_viterbi(params, corpus, anneal));
        }
        else if(estimator == "T")
            sampler = model_ptr(new estimator::batch_bigram_tree(params, corpus, anneal));
        else if(estimator == "D")
            std::cerr
                << "D(ecayed Flip) estimator cannot be used in batch mode."
                << std::endl;
    }

    // bigram online sampler
    else if(ngram == 2 and mode == "online")
    {
        if(estimator == "F")
            std::cerr
                << "Error: F(lip) estimator cannot be used in online mode."
                << std::endl;
        else if(estimator == "V")
            sampler = model_ptr(new estimator::online_bigram_viterbi(params, corpus, anneal));
        else if(estimator == "T")
            sampler = model_ptr(new estimator::online_bigram_tree(params, corpus, anneal));
        else if(estimator == "D")
        {
            if(debug_level >= 1000)
                std::cout
                    << "Creating Bigram DecayedMCMC model, with decay rate " << decay_rate
                    << " and samples per utterance " << samples_per_utt << std::endl;

            sampler = model_ptr(
                new estimator::online_bigram_dmcmc(params, corpus, anneal, forget_rate, decay_rate, samples_per_utt));
        }
    }
    else if(ngram == 1 and mode == "batch")
    {
        if(estimator == "F")
            sampler = model_ptr(new estimator::batch_unigram_flip(params, corpus, anneal));
        else if(estimator == "V")
            sampler = model_ptr(new estimator::batch_unigram_viterbi(params, corpus, anneal));
        else if(estimator == "T")
            sampler = model_ptr(new estimator::batch_unigram_tree(params, corpus, anneal));
        else if(estimator == "D")
            std::cerr << "D(ecayed Flip) estimator cannot be used in batch mode." << std::endl;
    }

    else if(ngram == 1 and mode == "online")
    {
        if(estimator == "F")
            std::cerr
                << "Error: F(lip) estimator cannot be used in online mode."
                << std::endl;
        else if(estimator == "V")
            sampler = model_ptr(new estimator::online_unigram_viterbi(params, corpus, anneal, forget_rate));
        else if(estimator == "T")
            sampler = model_ptr(new estimator::online_unigram_tree(params, corpus, anneal, forget_rate));
        else if(estimator == "D")
        {
            if(debug_level >= 1000)
                std::cout
                    << "Creating Unigram DecayedMCMC model, with decay rate "
                    << decay_rate << " and samples per utterance " << samples_per_utt
                    << std::endl;

            sampler = model_ptr(
                new estimator::online_unigram_dmcmc(params, corpus, anneal, forget_rate, decay_rate, samples_per_utt));
        }
    }

    return sampler;
}
