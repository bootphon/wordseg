#ifndef _ESTIMATOR_FACTORY_H
#define _ESTIMATOR_FACTORY_H

#include <memory>
#include <string>

#include "estimator/base.hh"
#include "corpus/corpus_data.hh"


namespace estimator
{
    std::shared_ptr<estimator::base> get_estimator(
        const parameters& params,
        const corpus::corpus_data& data,
        const annealing& anneal,
        const std::size_t ngram,
        const std::string& mode,
        const std::string& estimator,
        const double forget_rate,
        const double decay_rate,
        const std::size_t samples_per_utt);
}


#endif  // _ESTIMATOR_FACTORY_H
