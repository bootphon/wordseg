#include "sampler.hh"

#include "sampler/dmcmc.hh"
#include "sampler/flip.hh"
#include "sampler/tree.hh"
#include "sampler/viterbi.hh"

#include <iostream>


std::shared_ptr<sampler::base> sampler::get_sampler(
    CorpusData* data,
    const uint ngram,
    const std::string& mode,
    const std::string& estimator,
    const double forget_rate,
    const double decay_rate,
    const uint samples_per_utt)
{
    std::cout << "Init sampler with ngram=" << ngram << ", " << mode << ", " << estimator << std::endl;

    using model_ptr = std::shared_ptr<sampler::base>;
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
            TRACE(data->nsentences());
            TRACE(data->get_sentences());
            TRACE(data->get_eval_sentences());
            // TRACE(data->nchartypes());

            sampler = model_ptr(new sampler::batch_bigram_flip(data));
        }
        else if(estimator == "V")
        {
            sampler = model_ptr(new sampler::batch_bigram_viterbi(data));
        }
        else if(estimator == "T")
            sampler = model_ptr(new sampler::batch_bigram_tree(data));
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
            sampler = model_ptr(new sampler::online_bigram_viterbi(data));
        else if(estimator == "T")
            sampler = model_ptr(new sampler::online_bigram_tree(data));
        else if(estimator == "D")
        {
            if(debug_level >= 1000)
                std::cout
                    << "Creating Bigram DecayedMCMC model, with decay rate " << decay_rate
                    << " and samples per utterance " << samples_per_utt << std::endl;

            sampler = model_ptr(
                new sampler::online_bigram_dmcmc(data, forget_rate, decay_rate, samples_per_utt));
        }
    }
    else if(ngram == 1 and mode == "batch")
    {
        if(estimator == "F")
            sampler = model_ptr(new sampler::batch_unigram_flip(data));
        else if(estimator == "V")
            sampler = model_ptr(new sampler::batch_unigram_viterbi(data));
        else if(estimator == "T")
            sampler = model_ptr(new sampler::batch_unigram_tree(data));
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
            sampler = model_ptr(new sampler::online_unigram_viterbi(data, forget_rate));
        else if(estimator == "T")
            sampler = model_ptr(new sampler::online_unigram_tree(data, forget_rate));
        else if(estimator == "D")
        {
            if(debug_level >= 1000)
                std::cout
                    << "Creating Unigram DecayedMCMC model, with decay rate "
                    << decay_rate << " and samples per utterance " << samples_per_utt
                    << std::endl;

            sampler = model_ptr(
                new sampler::online_unigram_dmcmc(data, forget_rate, decay_rate, samples_per_utt));
        }
    }

    return sampler;
}
