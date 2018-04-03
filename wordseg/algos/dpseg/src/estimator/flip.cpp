#include "estimator/flip.hh"


estimator::batch_unigram_flip::batch_unigram_flip(
    const parameters& params, const text::corpus_base& corpus, const annealing& anneal)
    : batch_unigram(params, corpus, anneal)
{
    if(debug_level >= 1000)
        std::wcout << "BatchUnigramFlipestimator::Printing current _lex:"
                   << std::endl << m_lex << std::endl;
}


estimator::batch_unigram_flip::~batch_unigram_flip()
{}


void estimator::batch_unigram_flip::estimate_sentence(sentence& s, double temperature)
{
    s.sample_by_flips(m_lex, temperature);
}


estimator::batch_bigram_flip::batch_bigram_flip(
    const parameters& params, const text::corpus_base& corpus, const annealing& anneal)
    : batch_bigram(params, corpus, anneal)
{}


estimator::batch_bigram_flip::~batch_bigram_flip()
{}


void estimator::batch_bigram_flip::estimate_sentence(sentence& s, double temperature)
{
    s.sample_by_flips(m_lex, temperature);
}
