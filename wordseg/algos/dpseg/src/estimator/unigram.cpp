#include "estimator/unigram.hh"


estimator::unigram::unigram(const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal)
    : base(params, corpus, anneal),
      m_lex(m_base_dist, unif01, m_params.a1(), m_params.b1())
{}


estimator::unigram::~unigram()
{}


bool estimator::unigram::sanity_check() const
{
    bool sane = base::sanity_check();
    assert(_lex.ntokens() >= _nsentences_seen);
    sane = sane && m_lex.sanity_check();
    return sane;
}


double estimator::unigram::log_posterior() const
{
    return base::log_posterior(m_lex);
}


std::vector<double> estimator::unigram::predict_pairs(
    const std::vector<std::pair<substring, substring> >& test_pairs) const
{
    return base::predict_pairs(test_pairs, m_lex);
}

void estimator::unigram::print_lexicon(std::wostream& os) const
{
    os << "Unigram lexicon:" << std::endl;
    m_lex.print(os);
}


std::vector<bool> estimator::unigram::hypersample(double temperature)
{
    return base::hypersample(m_lex, temperature);
}


void estimator::unigram::print_statistics(std::wostream& os, std::size_t iter, double temp, bool header)
{
    if (header)
    {
        os << "#Iter"
           << sep << "Temp"
           << sep << "-logP"
           << sep << "a1" << sep << "b1" << sep << "Pstop"
           << std::endl;
    }

    os << iter << sep << temp << sep << -log_posterior() << sep
       << m_lex.pya() << sep << m_lex.pyb() << sep << m_lex.base_dist().p_stop()
       << " ";
}


void estimator::unigram::estimate_eval_sentence(sentence& s, double temperature, bool maximize)
{
    if (maximize)
        s.maximize(
            m_lex, m_corpus.nsentences()-1, temperature, m_params.do_mbdp());
    else
        s.sample_tree(
            m_lex, m_corpus.nsentences()-1, temperature, m_params.do_mbdp());
}


estimator::batch_unigram::batch_unigram(
    const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal)
    : unigram(params, corpus, anneal)
{
    for(const auto& sent: m_sentences)
    {
        // if (debug_level >= 10000) sent.print(std::wcerr);
        sent.insert_words(m_lex);
    }

    if (debug_level >= 10000) std::wcerr << m_lex << std::endl;
}


estimator::batch_unigram::~batch_unigram()
{}


void estimator::batch_unigram::estimate(
    std::size_t iters, std::wostream& os, std::size_t eval_iters, double temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
    {
        std::wcout << "Inside BatchUnigram estimate " << std::endl;
    }

    if (m_params.trace_every() > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    //number of accepts in hyperparm resampling. Init to correct length.
    std::vector<bool> accepted_anneal(hypersample(1).size());
    std::vector<bool> accepted(hypersample(1).size());
    std::size_t nanneal = 0;
    std::size_t n = 0;
    for (std::size_t i=1; i <= iters; i++)
    {
        //std::size_t nchanged = 0; // if need to print out, use later
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

        if (m_params.trace_every() > 0 and i%m_params.trace_every() == 0)
        {
            print_statistics(os, i, temperature);
        }
        assert(sanity_check());
    }

    os << "hyperparm accept rate: ";
    if (m_params.hypersampling_ratio())
        os << accepted_anneal/nanneal << " (during annealing), "
           << accepted/n << " (after)" << std::endl;
    else
        os << "no hyperparm sampling" << std::endl;
}


estimator::online_unigram::online_unigram(
    const parameters& params, const corpus::corpus_base& corpus, const annealing& anneal, double forget_rate)
    : unigram(params, corpus, anneal),
      m_forget_rate(forget_rate)
{
    base::m_nsentences_seen = 0;
}


estimator::online_unigram::~online_unigram()
{}


void estimator::online_unigram::estimate(
    std::size_t iters, std::wostream& os, std::size_t eval_iters, double temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        std::wcout << "Inside OnlineUnigram estimate " << std::endl;

    if (m_params.trace_every() > 0)
    {
        print_statistics(os, 0, 0, true);
    }
    m_nsentences_seen = 0;
    for (std::size_t i=1; i <= iters; i++)
    {
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

        for(std::vector<sentence>::iterator iter = m_sentences.begin(); iter != m_sentences.end(); ++iter)
        {
            if (debug_level >= 10000) iter->print(std::wcerr);
            if(!is_decayed)
            {
		forget_items(iter);
            }

            // if (_nsentences_seen % 100 == 0) wcerr << ".";
            if (eval_iters && (m_nsentences_seen % eval_iters == 0))
            {
		os << "Test set after " << m_nsentences_seen
                   << " sentences of training " << std::endl;

		// run evaluation over test set
		run_eval(os,temp,maximize);
            }

            // add current sentence to _sentences_seen
            m_sentences_seen.push_back(*iter);
            estimate_sentence(*iter, temperature);
            m_nsentences_seen++;

            if (debug_level >= 9000) TRACE2(-log_posterior(), m_lex.ntokens());
            if (debug_level >= 15000)  TRACE2(m_lex.ntypes(),m_lex);
        }

        if (m_params.trace_every() > 0 and i % m_params.trace_every() == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }
}


void estimator::online_unigram::forget_items(std::vector<sentence>::iterator iter)
{
    if (m_forget_rate)
    {
        assert(! m_corpus.token_memory and ! m_corpus.type_memory);
        if (m_sentences.begin() + m_forget_rate <= iter)
        {
            if (debug_level >= 9000)
                TRACE(*(iter - m_forget_rate));

            (iter - m_forget_rate)->erase_words(m_lex);
            m_nsentences_seen--;
        }
        assert(sanity_check());
    }
    else if (m_params.type_memory())
    {
        while (m_lex.ntypes() > m_params.type_memory())
        {
            if (m_params.forget_method() == "U")
            {
                m_lex.erase_type_uniform();
            }
            else if (m_params.forget_method() == "P")
            {
                m_lex.erase_type_proportional();
            }
            else
            {
                error("unknown unigram type forget-method");
            }

            if (debug_level >= 15000)
                TRACE2(m_lex.ntypes(),m_lex);
        }

        // need #sents <= ntoks for math to work.
        if (m_lex.ntokens() < m_nsentences_seen)
        {
            m_nsentences_seen = m_lex.ntokens();
        }
        assert(sanity_check());
    }
    else if (m_params.token_memory())
    {
        while (m_lex.ntokens() > m_params.token_memory())
        {
            if (m_params.forget_method() == "U")
            {
                m_lex.erase_token_uniform();
            }
            else
            {
                error("unknown unigram token forget-method");
            }

            if (debug_level >= 15000)  TRACE2(m_lex.ntokens(),m_lex);
        }

        // need #sents <= ntoks for math to work.
        if (m_lex.ntokens() < m_nsentences_seen)
        {
            m_nsentences_seen = m_lex.ntokens();
        }
        assert(sanity_check());
    }
    else if (m_params.token_memory())
    {
        error("unigram forgetting scheme not yet implemented");
    }
}
