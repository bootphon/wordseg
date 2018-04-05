#include "estimator/tree.hh"


estimator::batch_unigram_tree::batch_unigram_tree(
    const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal)
    : batch_unigram(params, corpus, anneal)
{}

estimator::batch_unigram_tree::~batch_unigram_tree()
{}

void estimator::batch_unigram_tree::estimate_sentence(sentence& s, double temperature)
{
    s.erase_words(m_lex);
    s.sample_tree(m_lex, m_corpus.nsentences()-1, temperature, m_params.do_mbdp());
    s.insert_words(m_lex);
}


estimator::batch_bigram_tree::batch_bigram_tree(
    const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal)
    : batch_bigram(params, corpus, anneal)
{}

estimator::batch_bigram_tree::~batch_bigram_tree()
{}

void estimator::batch_bigram_tree::estimate_sentence(sentence& s, double temperature)
{
    s.erase_words(m_lex);
    s.sample_tree(m_lex, m_corpus.nsentences()-1, temperature);
    s.insert_words(m_lex);
}



estimator::online_bigram_tree::online_bigram_tree(
    const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal)
    : online_bigram(params, corpus, anneal)
{}

estimator::online_bigram_tree::~online_bigram_tree()
{}

void estimator::online_bigram_tree::estimate_sentence(sentence& s, double temperature)
{
    s.sample_tree(m_lex, m_nsentences_seen, temperature);
    s.insert_words(m_lex);
}


estimator::online_unigram_tree::online_unigram_tree(
    const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal, double forget_rate)
    : online_unigram(params, corpus, anneal, forget_rate)
{}

estimator::online_unigram_tree::~online_unigram_tree()
{}

void estimator::online_unigram_tree::estimate_sentence(sentence& s, double temperature)
{
    s.sample_tree(m_lex, m_nsentences_seen, temperature, m_params.do_mbdp());
    s.insert_words(m_lex);
}
