#include "sampler/viterbi.hh"


sampler::batch_unigram_viterbi::batch_unigram_viterbi(data::data* constants)
    : batch_unigram(constants)
{}

sampler::batch_unigram_viterbi::~batch_unigram_viterbi()
{}

void sampler::batch_unigram_viterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.maximize(_lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
    s.insert_words(_lex);
}


sampler::batch_bigram_viterbi::batch_bigram_viterbi(data::data* constants)
    : batch_bigram(constants)
{}

sampler::batch_bigram_viterbi::~batch_bigram_viterbi()
{}

void sampler::batch_bigram_viterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.erase_words(_lex);
    s.maximize(_lex, _constants->nsentences()-1, temperature);
    s.insert_words(_lex);
}




sampler::online_unigram_viterbi::online_unigram_viterbi(data::data* constants, F forget_rate)
    : online_unigram(constants, forget_rate)
{}

sampler::online_unigram_viterbi::~online_unigram_viterbi()
{}

void sampler::online_unigram_viterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.maximize(_lex, _nsentences_seen, temperature,_constants->do_mbdp);
    s.insert_words(_lex);
}


sampler::online_bigram_viterbi::online_bigram_viterbi(data::data* constants)
    : online_bigram(constants)
{}

sampler::online_bigram_viterbi::~online_bigram_viterbi()
{}

void sampler::online_bigram_viterbi::estimate_sentence(Sentence& s, F temperature)
{
    s.maximize(_lex, _nsentences_seen, temperature);
    s.insert_words(_lex);
}
