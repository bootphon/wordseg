#include "sampler/tree.hh"


sampler::batch_unigram_tree::batch_unigram_tree(data::data* constants)
    : batch_unigram(constants)
{}

sampler::batch_unigram_tree::~batch_unigram_tree()
{}

void sampler::batch_unigram_tree::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.sample_tree(_lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
    s.insert_words(_lex);
}


sampler::batch_bigram_tree::batch_bigram_tree(data::data* constants)
    : batch_bigram(constants)
{}

sampler::batch_bigram_tree::~batch_bigram_tree()
{}

void sampler::batch_bigram_tree::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.sample_tree(_lex, _constants->nsentences()-1, temperature);
    s.insert_words(_lex);
}



sampler::online_bigram_tree::online_bigram_tree(data::data* constants)
    : online_bigram(constants)
{}

sampler::online_bigram_tree::~online_bigram_tree()
{}

void sampler::online_bigram_tree::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_tree(_lex, _nsentences_seen, temperature);
    s.insert_words(_lex);
}


sampler::online_unigram_tree::online_unigram_tree(data::data* constants, F forget_rate)
    : online_unigram(constants, forget_rate)
{}

sampler::online_unigram_tree::~online_unigram_tree()
{}

void sampler::online_unigram_tree::estimate_sentence(Sentence& s, F temperature)
{
    s.sample_tree(_lex, _nsentences_seen, temperature,_constants->do_mbdp);
    s.insert_words(_lex);
}
