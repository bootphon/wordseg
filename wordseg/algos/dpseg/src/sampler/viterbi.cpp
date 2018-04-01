#include "sampler/viterbi.hh"


sampler::batch_unigram_viterbi::batch_unigram_viterbi(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : batch_unigram(params, constants, anneal)
{}

sampler::batch_unigram_viterbi::~batch_unigram_viterbi()
{}

void sampler::batch_unigram_viterbi::estimate_sentence(Sentence& s, double temperature)
{
    s.erase_words(m_lex);
    s.maximize(m_lex, m_constants.nsentences()-1, temperature, m_params.do_mbdp);
    s.insert_words(m_lex);
}


sampler::batch_bigram_viterbi::batch_bigram_viterbi(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : batch_bigram(params, constants, anneal)
{}

sampler::batch_bigram_viterbi::~batch_bigram_viterbi()
{}

void sampler::batch_bigram_viterbi::estimate_sentence(Sentence& s, double temperature)
{
    s.erase_words(m_lex);
    s.maximize(m_lex, m_constants.nsentences()-1, temperature);
    s.insert_words(m_lex);
}




sampler::online_unigram_viterbi::online_unigram_viterbi(
    const parameters& params, const data::data& constants, const annealing& anneal, double forget_rate)
    : online_unigram(params, constants, anneal, forget_rate)
{}

sampler::online_unigram_viterbi::~online_unigram_viterbi()
{}

void sampler::online_unigram_viterbi::estimate_sentence(Sentence& s, double temperature)
{
    s.maximize(m_lex, m_nsentences_seen, temperature, m_params.do_mbdp);
    s.insert_words(m_lex);
}


sampler::online_bigram_viterbi::online_bigram_viterbi(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : online_bigram(params, constants, anneal)
{}

sampler::online_bigram_viterbi::~online_bigram_viterbi()
{}

void sampler::online_bigram_viterbi::estimate_sentence(Sentence& s, double temperature)
{
    s.maximize(m_lex, m_nsentences_seen, temperature);
    s.insert_words(m_lex);
}
