#include "sampler/flip.hh"


sampler::batch_unigram_flip::batch_unigram_flip(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : batch_unigram(params, constants, anneal)
{
    if(debug_level >= 1000)
        std::wcout << "BatchUnigramFlipSampler::Printing current _lex:"
                   << std::endl << m_lex << std::endl;
}


sampler::batch_unigram_flip::~batch_unigram_flip()
{}


void sampler::batch_unigram_flip::estimate_sentence(Sentence& s, double temperature)
{
    s.sample_by_flips(m_lex, temperature);
}


sampler::batch_bigram_flip::batch_bigram_flip(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : batch_bigram(params, constants, anneal)
{}


sampler::batch_bigram_flip::~batch_bigram_flip()
{}


void sampler::batch_bigram_flip::estimate_sentence(Sentence& s, double temperature)
{
    s.sample_by_flips(m_lex, temperature);
}
