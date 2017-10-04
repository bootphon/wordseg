#ifndef _BATCHSAMPLER_H_
#define _BATCHSAMPLER_H_

#include "Sentence.h"
#include "Unigrams.h"
#include "Data.h"
#include "slice-sampler.h"
#include "Scoring.h"

extern std::wstring sep;  //!< separator used to separate fields during printing of results


inline void error(const char *s)
{
    std::cerr << "error: " << s << std::endl; abort(); exit(1);
}

inline void error(const std::string s)
{
    error(s.c_str());
}


class ModelBase
{
public:
    ModelBase(Data*);

    virtual ~ModelBase();

    virtual bool sanity_check() const = 0;

    virtual F log_posterior() const = 0;

    virtual void estimate(
        U iters, std::wostream& os, U eval_iters = 0,
        F temperature = 1, bool maximize = false, bool is_decayed = false) = 0;

    virtual void run_eval(std::wostream& os, F temperature = 1, bool maximize = false) = 0;

    virtual Fs predict_pairs(const TestPairs& test_pairs) const = 0;

    virtual void print_segmented(std::wostream& os) const = 0;
    virtual void print_lexicon(std::wostream& os) const = 0;
    virtual void print_scores(std::wostream& os) = 0;

protected:
    Data* _constants;
    Sentences _sentences;
    Sentences _eval_sentences;
    U _nsentences_seen;
    Scoring _scoring;

    void resample_pya(Unigrams& lex);
    void resample_pyb(Unigrams& lex);

    virtual Bs hypersample(Unigrams& lex, F temperature);
    virtual Bs hypersample(Unigrams& ulex, Bigrams& lex, F temperature);
    virtual bool sample_hyperparm(F& beta, bool is_prob, F temperature);

    F log_posterior(const Unigrams& lex) const;
    F log_posterior(const Unigrams& ulex, const Bigrams& lex) const;

    Fs predict_pairs(const TestPairs& test_pairs, const Unigrams& lex) const
        {
            Fs probs;
            for(const auto& tp: test_pairs)
            {
                F p1 = lex(tp.first);
                F p2 = lex(tp.second);
                if (debug_level >= 10000) TRACE2(p1, p2);
                probs.push_back(p1 / (p1 + p2));
            }

            return probs;
        }

    Fs predict_pairs(const TestPairs& test_pairs, const Bigrams& lex) const
        {
            Fs probs;
            error("ModelBase::predict_pairs is not implemented for bigram models\n");
            return probs;
        }

    void print_segmented_sentences(std::wostream& os, const Sentences& sentences) const
        {
            for(const auto& item: sentences)
                os << item << std::endl;
        }

    void print_scores_sentences(std::wostream& os, const Sentences& sentences)
        {
            _scoring.reset();
            for(const auto& item: sentences)
                item.score(_scoring);
            _scoring.print_results(os);
        }
};

class Model: public ModelBase {
public:
    Model(Data* constants):
        ModelBase(constants),
        _base_dist(_constants->Pstop, _constants->nchartypes) {}
    virtual ~Model() {}
    virtual bool sanity_check() const
        {
            assert(_base_dist.nchartypes() ==_constants->nchartypes);
            assert(_base_dist.p_stop() < 0 || // if we're learning this parm.
                   _base_dist.p_stop() ==_constants->Pstop);
            return true;
        }

    virtual F log_posterior() const = 0;

    //use whatever sampling method is in subclass to segment training data
    virtual void estimate(
        U iters, std::wostream& os, U eval_iters = 0,
        F temperature = 1, bool maximize = false, bool is_decayed = false) = 0;

    //make single pass through test data, segmenting based on sampling
    //or maximization of each utt, using current counts from training
    //data only (i.e. no new counts are added)
    virtual void run_eval(std::wostream& os, F temperature = 1, bool maximize=false);

    virtual void print_segmented(std::wostream& os) const
        {
            print_segmented_sentences(os, _sentences);
        }

    virtual void print_eval_segmented(std::wostream& os) const
        {
            print_segmented_sentences(os, _eval_sentences);
        }

    //recomputes and prints precision, recall, etc. on training data
    void print_scores(std::wostream& os)
        {
            print_scores_sentences(os, _sentences);
        }

    //recomputes and prints precision, recall, etc. on training data
    void print_eval_scores(std::wostream& os)
        {
            print_scores_sentences(os, _eval_sentences);
        }

protected:
    P0 _base_dist;
    virtual void print_statistics(std::wostream& os, U iters, F temp, bool do_header=false) = 0;
    virtual void estimate_sentence(Sentence& s, F temperature) = 0;
    virtual void estimate_eval_sentence(Sentence& s, F temperature, bool maximize = false) = 0;
};

class UnigramModel: public Model {
public:
    UnigramModel(Data* constants):
        Model(constants),
        _lex(_base_dist, unif01, _constants->a1, _constants->b1)
        {}

    virtual ~UnigramModel() {}

    virtual bool sanity_check() const
        {
            bool sane = Model::sanity_check();
            assert(_lex.ntokens() >= _nsentences_seen);
            sane = sane && _lex.sanity_check();
            return sane;
        }

    virtual F log_posterior() const
        {
            return ModelBase::log_posterior(_lex);
        }

    virtual void estimate(
        U iters, std::wostream& os, U eval_iters = 0,
        F temperature = 1, bool maximize = false, bool is_decayed = false) = 0;

    virtual Fs predict_pairs(const TestPairs& test_pairs) const
        {
            return ModelBase::predict_pairs(test_pairs, _lex);
        }

    virtual void print_lexicon(std::wostream& os) const
        {
            os << "Unigram lexicon:" << std::endl;
            _lex.print(os);
        }

    //Unigrams get_lex(){return _lex;};

protected:
    Unigrams _lex;
    virtual void print_statistics(std::wostream& os, U iters, F temp, bool do_header=false);
    virtual Bs hypersample(F temperature)
        {
            return ModelBase::hypersample(_lex, temperature);
        }

    virtual void estimate_sentence(Sentence& s, F temperature) = 0;
    virtual void estimate_eval_sentence(Sentence& s, F temperature, bool maximize = false);
};


class BigramModel: public Model {
public:
  BigramModel(Data* constants):
    Model(constants),
    _ulex(_base_dist, unif01, _constants->a1, _constants->b1),
    _lex(_ulex, unif01, _constants->a2, _constants->b2) {
  }
  virtual ~BigramModel() {}
  virtual bool sanity_check() const {
    bool sane = Model::sanity_check();
    sane = sane && _ulex.sanity_check();
    sane = sane && _lex.sanity_check();
    //    sane = sane && _lex.get_a() ==  _constants->a2;
    //    sane = sane && _lex.get_b() ==  _constants->b2;
    return sane;
  }
  virtual F log_posterior() const {
    return ModelBase::log_posterior(_ulex, _lex);
  }
  virtual void estimate(U iters, std::wostream& os, U eval_iters = 0,
						F temperature = 1, bool maximize = false, bool is_decayed = false) = 0;
  virtual Fs predict_pairs(const TestPairs& test_pairs) const {
    return ModelBase::predict_pairs(test_pairs, _lex);
  }
  virtual void print_lexicon(std::wostream& os) const {
    os << "Unigram lexicon:" << std::endl;
    _ulex.print(os);
  }
protected:
  Unigrams _ulex;
  Bigrams _lex;
  virtual void print_statistics(std::wostream& os, U iters, F temp, bool do_header=false);
  virtual Bs hypersample(F temperature){
    return ModelBase::hypersample(_ulex, _lex, temperature);
  }
  virtual void estimate_sentence(Sentence& s, F temperature) = 0;
  virtual void estimate_eval_sentence(Sentence& s, F temperature, bool maximize = false);
};

class BatchUnigram: public UnigramModel {
public:
  BatchUnigram(Data* constants);
  virtual ~BatchUnigram() {}
  virtual void estimate(U iters, std::wostream& os, U eval_iters = 0,
						F temperature = 1, bool maximize = false, bool is_decayed = false);
protected:
  virtual void estimate_sentence(Sentence& s, F temperature) = 0;
};

class BatchUnigramViterbi: public BatchUnigram {
public:
  BatchUnigramViterbi(Data* constants): BatchUnigram(constants) {}
  virtual ~BatchUnigramViterbi() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class BatchUnigramFlipSampler: public BatchUnigram {
public:
  BatchUnigramFlipSampler(Data* constants): BatchUnigram(constants) {
  if(debug_level >= 1000)
      std::wcout << "BatchUnigramFlipSampler::Printing current _lex:"
                 << std::endl << _lex << std::endl;
  }
  virtual ~BatchUnigramFlipSampler() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class BatchUnigramTreeSampler: public BatchUnigram {
public:
  BatchUnigramTreeSampler(Data* constants): BatchUnigram(constants) {}
  virtual ~BatchUnigramTreeSampler() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class OnlineUnigram: public UnigramModel {
public:
  OnlineUnigram(Data* constants, F forget_rate = 0):
    UnigramModel(constants), _forget_rate(forget_rate) {
    Model::_nsentences_seen = 0;}
  virtual ~OnlineUnigram() {}
  virtual void estimate(U iters, std::wostream& os, U eval_iters = 0,
						F temperature = 1, bool maximize = false, bool is_decayed = false);
protected:
  F _forget_rate;
  Sentences _sentences_seen; // for use with DeacyedMCMC model in particular
  virtual void estimate_sentence(Sentence& s, F temperature) = 0;
  void forget_items(Sentences::iterator i);
};

class OnlineUnigramViterbi: public OnlineUnigram {
public:
  OnlineUnigramViterbi(Data* constants, F forget_rate = 0):
    OnlineUnigram(constants, forget_rate) {}
  virtual ~OnlineUnigramViterbi() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class OnlineUnigramTreeSampler: public OnlineUnigram {
public:
  OnlineUnigramTreeSampler(Data* constants, F forget_rate = 0):
    OnlineUnigram(constants, forget_rate) {
	}
  virtual ~OnlineUnigramTreeSampler() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class DecayedMCMC{
public:
	DecayedMCMC(F decay_rate = 0, U samples_per_utt = 100);
	virtual ~DecayedMCMC(){}

protected:
  F _decay_rate;
  U _samples_per_utt;
  Fs _decay_offset_probs;
  F _cum_decay_prob;
  U _num_total_pot_boundaries;
  U _num_curr_pot_boundaries;
  Us _boundaries_num_sampled;
  U _boundary_within_sentence;
  Sentences::iterator _sentence_sampled;
  virtual void decayed_initialization(Sentences _sentences);
  virtual void calc_new_cum_prob(Sentence& s, U num_boundaries);
  virtual U find_boundary_to_sample();
  virtual void find_sent_to_sample(U b_to_sample, Sentence& to_sample, Sentences& sentences_seen);
  void replace_sampled_sentence(Sentence s, Sentences& sentences_seen);
};


class OnlineUnigramDecayedMCMC:public OnlineUnigram, public DecayedMCMC {
public:
  OnlineUnigramDecayedMCMC(Data* constants, F forget_rate = 0, F decay_rate = 1.0, U samples_per_utt = 1000);
  virtual ~OnlineUnigramDecayedMCMC() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);

};

class BatchBigram: public BigramModel {
public:
  BatchBigram(Data* constants);
  virtual ~BatchBigram() {}
  virtual void estimate(U iters, std::wostream& os, U eval_iters = 0,
						F temperature = 1, bool maximize = false, bool is_decayed = false);
protected:
  virtual void estimate_sentence(Sentence& s, F temperature) = 0;
};

class BatchBigramViterbi: public BatchBigram {
  public:
  BatchBigramViterbi(Data* constants): BatchBigram(constants) {}
  virtual ~BatchBigramViterbi() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class BatchBigramFlipSampler: public BatchBigram {
  public:
  BatchBigramFlipSampler(Data* constants): BatchBigram(constants) {}
  virtual ~BatchBigramFlipSampler() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class BatchBigramTreeSampler: public BatchBigram {
  public:
  BatchBigramTreeSampler(Data* constants): BatchBigram(constants) {}
  virtual ~BatchBigramTreeSampler() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class OnlineBigram: public BigramModel {
public:
  OnlineBigram(Data* constants, F forget_rate = 0):
    BigramModel(constants), _forget_rate(forget_rate) {
    Model::_nsentences_seen = 0;}
  virtual ~OnlineBigram() {}
  virtual void estimate(
      U iters, std::wostream& os, U eval_iters = 0,
      F temperature = 1, bool maximize = false, bool is_decayed = false);
protected:
  F _forget_rate;
  Sentences _sentences_seen; // for use with DeacyedMCMC model in particular
  virtual void estimate_sentence(Sentence& s, F temperature) = 0;
  void forget_items(Sentences::iterator i);
};

class OnlineBigramViterbi: public OnlineBigram {
public:
  OnlineBigramViterbi(Data* constants): OnlineBigram(constants) {}
  virtual ~OnlineBigramViterbi() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class OnlineBigramTreeSampler: public OnlineBigram {
  public:
  OnlineBigramTreeSampler(Data* constants): OnlineBigram(constants) {}
  virtual ~OnlineBigramTreeSampler() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

class OnlineBigramDecayedMCMC:public OnlineBigram, public DecayedMCMC {
public:
  OnlineBigramDecayedMCMC(Data* constants, F forget_rate = 0, F decay_rate = 1.0, U samples_per_utt = 1000);
  virtual ~OnlineBigramDecayedMCMC() {}
protected:
  virtual void estimate_sentence(Sentence& s, F temperature);
};

#endif
