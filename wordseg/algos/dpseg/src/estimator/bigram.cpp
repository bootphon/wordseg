#include "estimator/bigram.hh"


estimator::bigram::bigram(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal)
    : estimator::base(params, corpus, anneal),
      m_ulex(m_base_dist, unif01, m_params.a1(), m_params.b1()),
      m_lex(m_ulex, unif01, m_params.a2(), m_params.b2())
{}


estimator::bigram::~bigram()
{}


bool estimator::bigram::sanity_check() const
{
    bool sane = estimator::base::sanity_check();
    sane = sane && m_ulex.sanity_check();
    sane = sane && m_lex.sanity_check();
    //    sane = sane && _lex.get_a() ==  _constants.a2;
    //    sane = sane && _lex.get_b() ==  _constants.b2;
    return sane;
}

double estimator::bigram::log_posterior() const
{
    return estimator::base::log_posterior(m_ulex, m_lex);
}


std::vector<double> estimator::bigram::predict_pairs(
    const std::vector<std::pair<substring, substring> >& test_pairs) const
{
    return estimator::base::predict_pairs(test_pairs, m_lex);
}


void estimator::bigram::print_lexicon(std::wostream& os) const
{
    os << "Unigram lexicon:" << std::endl;
    m_ulex.print(os);
}


std::vector<bool> estimator::bigram::hypersample(double temperature)
{
    return estimator::base::hypersample(m_ulex, m_lex, temperature);
}


void estimator::bigram::print_statistics(std::wostream& os, std::size_t iter, double temp, bool header)
{
    if (header)
    {
        os << "#Iter"
           << sep << "Temp"
           << sep << "-logP"
           << sep << "a1" << sep << "b1" << sep << "Pstop"
           << sep << "a2" << sep << "b2"
           << std::endl;
    }

    os << iter << sep << temp << sep << -log_posterior() << sep
       << m_ulex.pya() << sep << m_ulex.pyb() << sep << m_ulex.base_dist().p_stop() << sep
       << m_lex.pya() << sep << m_lex.pyb()
       << " ";
}

void estimator::bigram::estimate_eval_sentence(sentence& s, double temperature, bool maximize)
{
    if (maximize)
    {
        s.maximize(m_lex, m_corpus.nsentences()-1, temperature);
    }
    else
    {
        s.sample_tree(m_lex, m_corpus.nsentences()-1, temperature);
    }
}


estimator::batch_bigram::batch_bigram(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal)
    : bigram(params, corpus, anneal)
{
    for(const auto& item: m_sentences)
    {
        if (debug_level >= 10000) item.print(std::wcerr);
        item.insert_words(m_lex);
    }

    if (debug_level >= 10000) std::wcerr << m_lex << std::endl;
}


estimator::batch_bigram::~batch_bigram()
{}


void estimator::batch_bigram::estimate(
    std::size_t iters, std::wostream& os, std::size_t eval_iters, double temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        std::wcout << "Inside BatchBigram estimate " << std::endl;

    if (m_params.trace_every() > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    // number of accepts in hyperparm resampling. Initialize to the
    // correct length
    std::vector<bool> accepted_anneal(hypersample(1).size());
    std::vector<bool> accepted(hypersample(1).size());
    std::size_t nanneal = 0;
    std::size_t n = 0;
    for (std::size_t i=1; i <= iters; i++)
    {
        //std::size_t nchanged = 0; if need to print out, un-comment
        double temperature = m_annealing.temperature(i);

        // if (i % 10 == 0) wcerr << ".";
	if (eval_iters && (i % eval_iters == 0))
        {
            os << "Test set after " << i << " iterations of training " << std::endl;

            // run evaluation over test set
            run_eval(os,temp,maximize);
	}

        for(auto& sent: m_sentences)
        {
            // if (debug_level >= 10000) sent.print(std::wcerr);
            estimate_sentence(sent, temperature);
            if (debug_level >= 9000) std::wcerr << m_lex << std::endl;
        }

        if (m_params.hypersampling_ratio())
        {
            if (temperature > 1)
            {
                accepted_anneal += hypersample(temperature);
                nanneal++;
            }
            else
            {
                accepted += hypersample(temperature);
                n++;
            }
        }

        if (m_params.trace_every() > 0 and i % m_params.trace_every() == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }

    os << "hyperparm accept rate: " << accepted_anneal/nanneal
       << " (during annealing), " << accepted/n << " (after)" << std::endl;
}


estimator::online_bigram::online_bigram(
    const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal, double forget_rate)
    : bigram(params, corpus, anneal)
{
    base::m_nsentences_seen = 0;
}


estimator::online_bigram::~online_bigram()
{}


void estimator::online_bigram::estimate(
    std::size_t iters, std::wostream& os, std::size_t eval_iters, double temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        std::wcout << "Inside OnlineBigram estimate " << std::endl;

    if (m_params.trace_every() > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    m_nsentences_seen = 0;
    for (std::size_t i=1; i <= iters; i++)
    {
        std::wcerr << "bigram::online_bigram::estimate iteration " << i << "/" << iters << std::endl;
        double temperature = m_annealing.temperature(i);
	if(!is_decayed)
        {
            // if (i % 10 == 0) wcerr << ".";
            if (eval_iters && (i % eval_iters == 0))
            {
                os << "Test set after " << i << " iterations of training " << std::endl;

                // run evaluation over test set
                run_eval(os,temp,maximize);
            }
	}

        for(auto iter = m_sentences.begin(); iter != m_sentences.end(); ++iter)
        {
            if (debug_level >= 10000)
                iter->print(std::wcerr);

            if(!is_decayed)
            {
		forget_items(iter);
            }

            // if (_nsentences_seen % 100 == 0) wcerr << ".";
            if (eval_iters && (m_nsentences_seen % eval_iters == 0))
            {
		os << "Test set after " << m_nsentences_seen << " sentences of training " << std::endl;

		// run evaluation over test set
		run_eval(os,temp,maximize);
            }

            // add current sentence to _sentences_seen
            m_sentences_seen.push_back(*iter);
            estimate_sentence(*iter, temperature);
            m_nsentences_seen++;
        }

        if (m_params.trace_every() > 0 and i % m_params.trace_every() == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }
}

void estimator::online_bigram::forget_items(std::vector<sentence>::iterator iter)
{
    if (m_forget_rate)
    {
        if (m_sentences.begin() + m_forget_rate <= iter)
        {
            if (debug_level >= 9000)
                TRACE(*(iter - m_forget_rate));

            (iter - m_forget_rate)->erase_words(m_lex);
            m_nsentences_seen--;
        }
    }
    else if (m_params.type_memory() || m_params.token_memory())
    {
        error("bigram forgetting scheme not yet implemented");
    }
}
