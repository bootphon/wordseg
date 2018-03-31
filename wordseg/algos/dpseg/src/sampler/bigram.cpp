#include "sampler/bigram.hh"


sampler::bigram::bigram(const data::data& constants)
    : sampler::base(constants),
      _ulex(_base_dist, unif01, constants.a1, constants.b1),
      _lex(_ulex, unif01, constants.a2, constants.b2)
{}



sampler::bigram::~bigram()
{}


bool sampler::bigram::sanity_check() const
{
    bool sane = sampler::base::sanity_check();
    sane = sane && _ulex.sanity_check();
    sane = sane && _lex.sanity_check();
    //    sane = sane && _lex.get_a() ==  _constants.a2;
    //    sane = sane && _lex.get_b() ==  _constants.b2;
    return sane;
}

F sampler::bigram::log_posterior() const
{
    return sampler::base::log_posterior(_ulex, _lex);
}


Fs sampler::bigram::predict_pairs(const TestPairs& test_pairs) const
{
    return sampler::base::predict_pairs(test_pairs, _lex);
}


void sampler::bigram::print_lexicon(std::wostream& os) const
{
    os << "Unigram lexicon:" << std::endl;
    _ulex.print(os);
}


Bs sampler::bigram::hypersample(F temperature)
{
    return sampler::base::hypersample(_ulex, _lex, temperature);
}


void sampler::bigram::print_statistics(std::wostream& os, U iter, F temp, bool header)
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
       << _ulex.pya() << sep << _ulex.pyb() << sep << _ulex.base_dist().p_stop() << sep
       << _lex.pya() << sep << _lex.pyb()
       << " ";

    print_scores(os);
}

void sampler::bigram::estimate_eval_sentence(Sentence& s, F temperature, bool maximize)
{
    if (maximize)
    {
        s.maximize(_lex, _constants.nsentences()-1, temperature);
    }
    else
    {
        s.sample_tree(_lex, _constants.nsentences()-1, temperature);
    }
}


sampler::batch_bigram::batch_bigram(const data::data& constants)
    : bigram(constants)
{
    for(const auto& item: _sentences)
    {
        if (debug_level >= 10000) item.print(std::wcerr);
        item.insert_words(_lex);
    }

    if (debug_level >= 10000) std::wcerr << _lex << std::endl;
}


sampler::batch_bigram::~batch_bigram()
{}


void sampler::batch_bigram::estimate(
    U iters, std::wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        std::wcout << "Inside BatchBigram estimate " << std::endl;

    if (_constants.trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    // number of accepts in hyperparm resampling. Initialize to the
    // correct length
    Bs accepted_anneal(hypersample(1).size());
    Bs accepted(hypersample(1).size());
    U nanneal = 0;
    U n = 0;
    for (U i=1; i <= iters; i++)
    {
        //U nchanged = 0; if need to print out, un-comment
        F temperature = _constants.anneal_temperature(i);

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

        if (_constants.hypersampling_ratio)
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

        if (_constants.trace_every > 0 and i%_constants.trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }

    os << "hyperparm accept rate: " << accepted_anneal/nanneal
       << " (during annealing), " << accepted/n << " (after)" << std::endl;
}


sampler::online_bigram::online_bigram(const data::data& constants, F forget_rate)
    : bigram(constants)
{
    base::_nsentences_seen = 0;
}


sampler::online_bigram::~online_bigram()
{}


void sampler::online_bigram::estimate(
    U iters, std::wostream& os, U eval_iters, F temp, bool maximize, bool is_decayed)
{
    if(debug_level >= 10000)
        std::wcout << "Inside OnlineBigram estimate " << std::endl;

    if (_constants.trace_every > 0)
    {
        print_statistics(os, 0, 0, true);
    }

    _nsentences_seen = 0;
    for (U i=1; i <= iters; i++)
    {
        std::wcerr << "bigram::online_bigram::estimate iteration " << i << "/" << iters << std::endl;
        F temperature = _constants.anneal_temperature(i);
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

        for(auto iter = _sentences.begin(); iter != _sentences.end(); ++iter)
        {
            if (debug_level >= 10000)
                iter->print(std::wcerr);

            if(!is_decayed)
            {
		forget_items(iter);
            }

            // if (_nsentences_seen % 100 == 0) wcerr << ".";
            if (eval_iters && (_nsentences_seen % eval_iters == 0))
            {
		os << "Test set after " << _nsentences_seen << " sentences of training " << std::endl;

		// run evaluation over test set
		run_eval(os,temp,maximize);
		print_eval_scores(std::wcout);
            }

            // add current sentence to _sentences_seen
            _sentences_seen.push_back(*iter);
            estimate_sentence(*iter, temperature);
            _nsentences_seen++;
        }

        if (_constants.trace_every > 0 and i%_constants.trace_every == 0)
        {
            print_statistics(os, i, temperature);
        }

        assert(sanity_check());
    }
}

void sampler::online_bigram::forget_items(Sentences::iterator iter)
{
    if (_forget_rate)
    {
        if (_sentences.begin()+_forget_rate <= iter)
        {
            if (debug_level >= 9000)
                TRACE(*(iter - _forget_rate));

            (iter-_forget_rate)->erase_words(_lex);
            _nsentences_seen--;
        }
    }
    else if (_constants.type_memory || _constants.token_memory)
    {
        error("bigram forgetting scheme not yet implemented");
    }
}
