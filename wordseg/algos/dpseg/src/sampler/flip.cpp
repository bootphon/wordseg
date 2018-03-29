#include "sampler/flip.hh"


sampler::batch_unigram_flip::batch_unigram_flip(data::data* constants)
    : batch_unigram(constants)
{
    if(debug_level >= 1000)
        std::wcout << "BatchUnigramFlipSampler::Printing current _lex:"
                   << std::endl << _lex << std::endl;
}


sampler::batch_unigram_flip::~batch_unigram_flip()
{}


void sampler::batch_unigram_flip::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_by_flips(_lex, temperature);
}


sampler::batch_bigram_flip::batch_bigram_flip(data::data* constants)
    : batch_bigram(constants)
{}


sampler::batch_bigram_flip::~batch_bigram_flip()
{}


void sampler::batch_bigram_flip::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_by_flips(_lex, temperature);
}
