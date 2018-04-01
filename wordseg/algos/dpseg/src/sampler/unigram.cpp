#include "sampler/unigram.hh"


sampler::unigram::unigram(const parameters& params, const data::data& constants, const annealing& anneal)
    : base(params, constants, anneal),
      m_lex(m_base_dist, unif01, m_params.a1, m_params.b1)
{}


sampler::unigram::~unigram()
{}


bool sampler::unigram::sanity_check() const
{
    bool sane = base::sanity_check();
    assert(_lex.ntokens() >= _nsentences_seen);
    sane = sane && m_lex.sanity_check();
    return sane;
}


double sampler::unigram::log_posterior() const
{
    return base::log_posterior(m_lex);
}


std::vector<double> sampler::unigram::predict_pairs(
    const std::vector<std::pair<substring, substring> >& test_pairs) const
{
    return base::predict_pairs(test_pairs, m_lex);
}

void sampler::unigram::print_lexicon(std::wostream& os) const
{
    os << "Unigram lexicon:" << std::endl;
    m_lex.print(os);
}


std::vector<bool> sampler::unigram::hypersample(double temperature)
{
    return base::hypersample(m_lex, temperature);
}


void sampler::unigram::print_statistics(std::wostream& os, uint iter, double temp, bool header)
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

    print_scores(os);
}


void sampler::unigram::estimate_eval_sentence(Sentence& s, double temperature, bool maximize)
{
    if (maximize)
        s.maximize(
            m_lex, m_constants.nsentences()-1, temperature, m_params.do_mbdp);
    else
        s.sample_tree(
            m_lex, m_constants.nsentences()-1, temperature, m_params.do_mbdp);
}


sampler::batch_unigram::batch_unigram(
    const parameters& params, const data::data& constants, const annealing& anneal)
    : unigram(params, constants, anneal)
{
    for(const auto& sent: m_sentences)
    {
        if (debug_level >= 10000) sent.print(std::wcerr);
        sent.insert_words(m_lex);
    }

    if (debug_level >= 10000) std::wcerr << m_lex << std::endl;
}


sampler::batch_unigram::~batch_unigram()
{}


void sampler::batch_unigram::estimate(
    uint iters, std::wostream& os, uint eval_iters, double temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
    {
        std::wcout << "Inside BatchUnigram estimate " << std::endl;
    }

    if (m_params.trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    //number of accepts in hyperparm resampling. Init to correct length.
    std::vector<bool> accepted_anneal(hypersample(1).size());
    std::vector<bool> accepted(hypersample(1).size());
    uint nanneal = 0;
    uint n = 0;
    for (uint i=1; i <= iters; i++)
    {
        //uint nchanged = 0; // if need to print out, use later
        double temperature = m_annealing.temperature(i);
        // if (i % 10 == 0) wcerr << ".";
	if (eval_iters && (i % eval_iters == 0))
        {
            os << "Test set after " << i << " iterations of training " << std::endl;

            // run evaluation over test set
            run_eval(os,temp,maximize);
            print_eval_scores(std::wcout);
	}

        for(auto& sent: m_sentences)
        {
            if (debug_level >= 10000) sent.print(std::wcerr);
            estimate_sentence(sent, temperature);
            if (debug_level >= 9000) std::wcerr << m_lex << std::endl;
        }

        if (m_params.hypersampling_ratio)
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

        if (m_params.trace_every > 0 and i%m_params.trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }
        assert(sanity_check());
    }

    os << "hyperparm accept rate: ";
    if (m_params.hypersampling_ratio)
        os << accepted_anneal/nanneal << " (during annealing), "
           << accepted/n << " (after)" << std::endl;
    else
        os << "no hyperparm sampling" << std::endl;
}


sampler::online_unigram::online_unigram(
    const parameters& params, const data::data& constants, const annealing& anneal, double forget_rate)
    : unigram(params, constants, anneal),
      _forget_rate(forget_rate)
{
    base::m_nsentences_seen = 0;
}


sampler::online_unigram::~online_unigram()
{}


void sampler::online_unigram::estimate(
    uint iters, std::wostream& os, uint eval_iters, double temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        std::wcout << "Inside OnlineUnigram estimate " << std::endl;

    if (m_params.trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }
    m_nsentences_seen = 0;
    for (uint i=1; i <= iters; i++)
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
                print_eval_scores(std::wcout);
            }
	}

        for(Sentences::iterator iter = m_sentences.begin(); iter != m_sentences.end(); ++iter)
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
		print_eval_scores(std::wcout);
            }

            // add current sentence to _sentences_seen
            _sentences_seen.push_back(*iter);
            estimate_sentence(*iter, temperature);
            m_nsentences_seen++;

            if (debug_level >= 9000) TRACE2(-log_posterior(), m_lex.ntokens());
            if (debug_level >= 15000)  TRACE2(m_lex.ntypes(),m_lex);
        }

        if (m_params.trace_every > 0 and i % m_params.trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }
}


void sampler::online_unigram::forget_items(Sentences::iterator iter)
{
    if (_forget_rate)
    {
        assert(! m_constants.token_memory and ! m_constants.type_memory);
        if (m_sentences.begin() + _forget_rate <= iter)
        {
            if (debug_level >= 9000)
                TRACE(*(iter-_forget_rate));

            (iter - _forget_rate)->erase_words(m_lex);
            m_nsentences_seen--;
        }
        assert(sanity_check());
    }
    else if (m_params.type_memory)
    {
        while (m_lex.ntypes() > m_params.type_memory)
        {
            if (m_params.forget_method == "U")
            {
                m_lex.erase_type_uniform();
            }
            else if (m_params.forget_method == "P")
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
    else if (m_params.token_memory)
    {
        while (m_lex.ntokens() > m_params.token_memory)
        {
            if (m_params.forget_method == "U")
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
    else if (m_params.token_memory)
    {
        error("unigram forgetting scheme not yet implemented");
    }
}
