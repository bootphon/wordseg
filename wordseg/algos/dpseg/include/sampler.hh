#ifndef _SAMPLER_H
#define _SAMPLER_H

#include <memory>
#include <string>
#include "sampler/base.hh"
#include "data/corpus_data.hh"


namespace sampler
{
    std::shared_ptr<sampler::base> get_sampler(
        data::corpus_data* data,
        const uint ngram,
        const std::string& mode,
        const std::string& estimator,
        const double forget_rate,
        const double decay_rate,
        const uint samples_per_utt);
}


#endif  // _SAMPLER_H
