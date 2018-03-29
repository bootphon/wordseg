#include "sampler/unigram.hh"


sampler::unigram::unigram(data::data* constants)
    : base(constants),
      _lex(_base_dist, unif01, constants->a1, constants->b1)
{}


sampler::unigram::~unigram()
{}


bool sampler::unigram::sanity_check() const
{
    bool sane = base::sanity_check();
    assert(_lex.ntokens() >= _nsentences_seen);
    sane = sane && _lex.sanity_check();
    return sane;
}


F sampler::unigram::log_posterior() const
{
    return base::log_posterior(_lex);
}


Fs sampler::unigram::predict_pairs(const TestPairs& test_pairs) const
{
    return base::predict_pairs(test_pairs, _lex);
}

void sampler::unigram::print_lexicon(std::wostream& os) const
{
    os << "Unigram lexicon:" << std::endl;
    _lex.print(os);
}


Bs sampler::unigram::hypersample(F temperature)
{
    return base::hypersample(_lex, temperature);
}


void sampler::unigram::print_statistics(std::wostream& os, U iter, F temp, bool header)
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
       << _lex.pya() << sep << _lex.pyb() << sep << _lex.base_dist().p_stop()
       << " ";

    print_scores(os);
}


void sampler::unigram::estimate_eval_sentence(Sentence& s, F temperature, bool maximize)
{
    if (maximize)
        s.maximize(
            _lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
    else
        s.sample_tree(
            _lex, _constants->nsentences()-1, temperature, _constants->do_mbdp);
}


sampler::batch_unigram::batch_unigram(data::data* constants)
    : unigram(constants)
{
    for(const auto& sent: _sentences)
    {
        if (debug_level >= 10000) sent.print(std::wcerr);
        sent.insert_words(_lex);
    }

    if (debug_level >= 10000) std::wcerr << _lex << std::endl;
}


sampler::batch_unigram::~batch_unigram()
{}


void sampler::batch_unigram::estimate(
    U iters, std::wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
    {
        std::wcout << "Inside BatchUnigram estimate " << std::endl;
    }

    if (_constants->trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    //number of accepts in hyperparm resampling. Init to correct length.
    Bs accepted_anneal(hypersample(1).size());
    Bs accepted(hypersample(1).size());
    U nanneal = 0;
    U n = 0;
    for (U i=1; i <= iters; i++)
    {
        //U nchanged = 0; // if need to print out, use later
        F temperature = _constants->anneal_temperature(i);
        // if (i % 10 == 0) wcerr << ".";
	if (eval_iters && (i % eval_iters == 0))
        {
            os << "Test set after " << i << " iterations of training " << std::endl;

            // run evaluation over test set
            run_eval(os,temp,maximize);
            print_eval_scores(std::wcout);
	}

        for(auto& sent: _sentences)
        {
            if (debug_level >= 10000) sent.print(std::wcerr);
            estimate_sentence(sent, temperature);
            if (debug_level >= 9000) std::wcerr << _lex << std::endl;
        }

        if (_constants->hypersampling_ratio)
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

        if (_constants->trace_every > 0 and i%_constants->trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }
        assert(sanity_check());
    }

    os << "hyperparm accept rate: ";
    if (_constants->hypersampling_ratio)
        os << accepted_anneal/nanneal << " (during annealing), "
           << accepted/n << " (after)" << std::endl;
    else
        os << "no hyperparm sampling" << std::endl;
}


sampler::online_unigram::online_unigram(data::data* constants, F forget_rate)
    : unigram(constants), _forget_rate(forget_rate)
{
    base::_nsentences_seen = 0;
}


sampler::online_unigram::~online_unigram()
{}


void sampler::online_unigram::estimate(
    U iters, std::wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        std::wcout << "Inside OnlineUnigram estimate " << std::endl;

    if (_constants->trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }
    _nsentences_seen = 0;
    for (U i=1; i <= iters; i++)
    {
        F temperature = _constants->anneal_temperature(i);
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

        for(Sentences::iterator iter = _sentences.begin(); iter != _sentences.end(); ++iter)
        {
            if (debug_level >= 10000) iter->print(std::wcerr);
            if(!is_decayed)
            {
		forget_items(iter);
            }

            // if (_nsentences_seen % 100 == 0) wcerr << ".";
            if (eval_iters && (_nsentences_seen % eval_iters == 0))
            {
		os << "Test set after " << _nsentences_seen
                   << " sentences of training " << std::endl;

		// run evaluation over test set
		run_eval(os,temp,maximize);
		print_eval_scores(std::wcout);
            }

            // add current sentence to _sentences_seen
            _sentences_seen.push_back(*iter);
            estimate_sentence(*iter, temperature);
            _nsentences_seen++;

            if (debug_level >= 9000) TRACE2(-log_posterior(), _lex.ntokens());
            if (debug_level >= 15000)  TRACE2(_lex.ntypes(),_lex);
        }

        if (_constants->trace_every > 0 and i%_constants->trace_every == 0)
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
        assert(!_constants->token_memory and !_constants->type_memory);
        if (_sentences.begin() + _forget_rate <= iter)
        {
            if (debug_level >= 9000)
                TRACE(*(iter-_forget_rate));

            (iter - _forget_rate)->erase_words(_lex);
            _nsentences_seen--;
        }
        assert(sanity_check());
    }
    else if (_constants->type_memory)
    {
        while (_lex.ntypes() >_constants->type_memory)
        {
            if (_constants->forget_method == "U")
            {
                _lex.erase_type_uniform();
            }
            else if (_constants->forget_method == "P")
            {
                _lex.erase_type_proportional();
            }
            else
            {
                error("unknown unigram type forget-method");
            }

            if (debug_level >= 15000)
                TRACE2(_lex.ntypes(),_lex);
        }

        // need #sents <= ntoks for math to work.
        if (_lex.ntokens() < _nsentences_seen)
        {
            _nsentences_seen = _lex.ntokens();
        }
        assert(sanity_check());
    }
    else if (_constants->token_memory)
    {
        while (_lex.ntokens() >_constants->token_memory)
        {
            if (_constants->forget_method == "U")
            {
                _lex.erase_token_uniform();
            }
            else
            {
                error("unknown unigram token forget-method");
            }

            if (debug_level >= 15000)  TRACE2(_lex.ntokens(),_lex);
        }

        // need #sents <= ntoks for math to work.
        if (_lex.ntokens() < _nsentences_seen)
        {
            _nsentences_seen = _lex.ntokens();
        }
        assert(sanity_check());
    }
    else if (_constants->token_memory)
    {
        error("unigram forgetting scheme not yet implemented");
    }
}
