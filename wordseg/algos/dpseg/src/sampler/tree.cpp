#include "sampler/tree.hh"


sampler::batch_unigram_tree::batch_unigram_tree(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : batch_unigram(params, constants, anneal)
{}

sampler::batch_unigram_tree::~batch_unigram_tree()
{}

void sampler::batch_unigram_tree::estimate_sentence(Sentence& s, double temperature)
{
    s.erase_words(m_lex);
    s.sample_tree(m_lex, m_constants.nsentences()-1, temperature, m_params.do_mbdp);
    s.insert_words(m_lex);
}


sampler::batch_bigram_tree::batch_bigram_tree(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : batch_bigram(params, constants, anneal)
{}

sampler::batch_bigram_tree::~batch_bigram_tree()
{}

void sampler::batch_bigram_tree::estimate_sentence(Sentence& s, double temperature)
{
    s.erase_words(m_lex);
    s.sample_tree(m_lex, m_constants.nsentences()-1, temperature);
    s.insert_words(m_lex);
}



sampler::online_bigram_tree::online_bigram_tree(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : online_bigram(params, constants, anneal)
{}

sampler::online_bigram_tree::~online_bigram_tree()
{}

void sampler::online_bigram_tree::estimate_sentence(Sentence& s, double temperature)
{
    s.sample_tree(m_lex, m_nsentences_seen, temperature);
    s.insert_words(m_lex);
}


sampler::online_unigram_tree::online_unigram_tree(
    const parameters& params, const data::data& constants, const annealing& anneal, double forget_rate)
    : online_unigram(params, constants, anneal, forget_rate)
{}

sampler::online_unigram_tree::~online_unigram_tree()
{}

void sampler::online_unigram_tree::estimate_sentence(Sentence& s, double temperature)
{
    s.sample_tree(m_lex, m_nsentences_seen, temperature, m_params.do_mbdp);
    s.insert_words(m_lex);
}
