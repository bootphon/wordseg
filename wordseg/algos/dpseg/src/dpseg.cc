#include <algorithm>
#include <chrono>
#include <iostream>
#include <locale>
#include <string>
#include <vector>


// with gcc below the version 5.0, the <codecvt> header of the
// standard library is not implemented, so we use the codecvt in
// boost::locale instead
#define BOOST_UTF8_BEGIN_NAMESPACE
#define BOOST_UTF8_END_NAMESPACE
#define BOOST_UTF8_DECL
#include <boost/detail/utf8_codecvt_facet.ipp>

#include <boost/program_options.hpp>

#include "random-mt19937ar.h"
#include "Estimators.h"
#include "Data.h"


// TODO The following are global variables accessed across the whole
// code (with 'extern' declarations). This is bad (source of bugs,
// hard to read/debug). Use parameters instead.

uniform01_type<double> unif01; // random number generator
unsigned int debug_level;      // higher -> mode debug messages on stdout
std::wstring sep;              // separator used to separate fields during printing of results


std::wstring str2wstr(std::string str)
{
    std::wstring temp_str(str.length(), L' ');  // Make room for characters
    std::copy(str.begin(), str.end(), std::back_inserter(temp_str));
    return temp_str;
}


Model* get_sampler(CorpusData* data,
                   const unsigned int ngram, const std::string& mode, const std::string& estimator,
                   const float forget_rate, const float decay_rate, const unsigned int samples_per_utt)
{
    Model* sampler = NULL;

    // make sure the ngram is 1 or 2
    if(ngram != 1 and ngram != 2)
    {
        std::cerr << "Error: ngram must be 1 or 2, it is " << ngram << std::endl;
        return NULL;
    }

    // make sure the mode is "batch" or "online"
    if(mode != "batch" and mode != "online")
    {
        std::cerr << "Error: invalid mode must be batch or online, it is " << mode << std::endl;
        return NULL;
    }

    // make sure the estimator is valid
    std::vector<std::string> estimators = {"F", "V", "T", "D"};
    if(std::find(estimators.begin(), estimators.end(), estimator) == estimators.end())
    {
        std::cerr << "Error: " << estimator << " is not a valid estimator" << std::endl;
        return NULL;
    }

    // bigram batch sampler
    if(ngram == 2 and mode == "batch")
    {
        if(estimator == "F")
            sampler = new BatchBigramFlipSampler(data);
        else if(estimator == "V")
            sampler = new BatchBigramViterbi(data);
        else if(estimator == "T")
            sampler = new BatchBigramTreeSampler(data);
        else if(estimator == "D")
            std::cerr
                << "D(ecayed Flip) estimator cannot be used in batch mode."
                << std::endl;
    }

    // bigram online sampler
    else if(ngram == 2 and mode == "online")
    {
        if(estimator == "F")
            std::cerr
                << "Error: F(lip) estimator cannot be used in online mode."
                << std::endl;
        else if(estimator == "V")
            sampler = new OnlineBigramViterbi(data);
        else if(estimator == "T")
            sampler = new OnlineBigramTreeSampler(data);
        else if(estimator == "D")
        {
            if(debug_level >= 1000)
                std::cout
                    << "Creating Bigram DecayedMCMC model, with decay rate " << decay_rate
                    << " and samples per utterance " << samples_per_utt << std::endl;

            sampler = new OnlineBigramDecayedMCMC(data, forget_rate, decay_rate, samples_per_utt);
        }
    }
    else if(ngram == 1 and mode == "batch")
    {
        if(estimator == "F")
            sampler = new BatchUnigramFlipSampler(data);
        else if(estimator == "V")
            sampler = new BatchUnigramViterbi(data);
        else if(estimator == "T")
            sampler = new BatchUnigramTreeSampler(data);
        else if(estimator == "D")
            std::cerr << "D(ecayed Flip) estimator cannot be used in batch mode." << std::endl;
    }

    else if(ngram == 1 and mode == "online")
    {
        if(estimator == "F")
            std::cerr
                << "Error: F(lip) estimator cannot be used in online mode."
                << std::endl;
        else if(estimator == "V")
            sampler = new OnlineUnigramViterbi(data, forget_rate);
        else if(estimator == "T")
            sampler = new OnlineUnigramTreeSampler(data, forget_rate);
        else if(estimator == "D")
        {
            if(debug_level >= 1000)
                std::cout
                    << "Creating Unigram DecayedMCMC model, with decay rate "
                    << decay_rate << " and samples per utterance " << samples_per_utt
                    << std::endl;

            sampler = new OnlineUnigramDecayedMCMC(data, forget_rate, decay_rate, samples_per_utt);
        }
    }

    return sampler;
}


int main(int argc, char** argv)
{
    // Set a new global UTF8 locale to make output streams handle utf8.
    // Otherwise we'll get aborts when trying to output large character
    // values.
    std::locale utf8_locale(std::locale(), new utf8_codecvt_facet());
    std::locale::global(utf8_locale);

    // std::wcerr.imbue(utf8_locale);
    // std::wcout.imbue(utf8_locale);
    std::wcout.precision(5);

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(0);

    CorpusData data;
    std::string csep;

    // define the command line arguments
    namespace po = boost::program_options;
    po::options_description desc("program_options");
    desc.add_options()
        ("help,h",
         "produce help message")

        ("config-file,C", po::value<std::string>(),
         "read options from this file")

        ("debug-level,d", po::value<U>(&debug_level)->default_value(0),
         "debugging level")

        ("data-file", po::value<std::string>(),
         "training data file (default is stdin)")

        ("data-start-index", po::value<U>()->default_value(0),
         "sentence index to start reading training data file")

        ("data-num-sents", po::value<U>()->default_value(0),
         "number of training sentences to use (0 = all)")

        ("eval-file", po::value<std::string>(),
         "testing data file (default is training file). "
         "If no eval-file is listed, evaluation will be on the training file. "
         "Note that listing the same file for both training and testing has "
         "different functionality than not listing a test file, due to the way "
         "that the test file is segmented.")

        ("eval-start-index", po::value<U>()->default_value(0),
         "sentence index to start reading eval data file")

        ("eval-num-sents", po::value<U>()->default_value(0),
         "number of testing sentences to use (0 = all)")

        ("eval-maximize", po::value<U>()->default_value(0),
         "1 = choose max prob segmentation of test sentences, 0 (default) = sample instead")

        ("eval-interval", po::value<U>()->default_value(0),
         "how many iterations are run before the test set is evaluated, 0 "
         "(default) means to only evaluate the test set after all iterations "
         "are complete.w")

        ("output-file,o", po::value<std::string>(),
         "segmented output file")

        ("estimator", po::value<std::string>()->default_value("F"),
         "possible values are: V(iterbi), F(lip), T(ree), D(ecayed Flip). "
         "Viterbi does dynamic programming maximization, Tree does dynamic "
         "programming sampling, Flip does original Gibbs sampler.")

        ("decay-rate", po::value<F>()->default_value(1.0),
         "decay rate for D(ecayed Flip), default = 1.0")

        ("samples-per-utt", po::value<U>()->default_value(1000),
         "samples per utterance for D(ecayed Flip), default = 1000")

        ("mode", po::value<std::string>()->default_value("batch"),
         "possible values are: online, batch")

        ("ngram", po::value<U>()->default_value(2),
         "possible values are: 1 (unigram), 2 (bigram)")

        ("do-mbdp", po::value<bool>(&data.do_mbdp)->default_value(false),
         "maximize using Brent ngram instead of DP")

        ("a1", po::value<F>(&data.a1)->default_value(0),
         "Unigram Pitman-Yor a parameter")

        ("b1", po::value<F>(&data.b1)->default_value(1),
         "Unigram Pitman-Yor b parameter")

        ("a2", po::value<F>(&data.a2)->default_value(0),
         "Bigram Pitman-Yor a parameter")

        ("b2", po::value<F>(&data.b2)->default_value(1),
         "Bigram Pitman-Yor b parameter")

        ("Pstop", po::value<F>(&data.Pstop)->default_value(0.5),
         "Monkey model stop probability")

        // not completely sure that hyper parameter sampling is
        // working correctly yet. However it does yield pretty good
        // results when using all four PY parameters.
        ("hypersamp-ratio", po::value<F>(&data.hypersampling_ratio)->default_value(0.1),
         "Standard deviation for new hyperparm proposals (0 turns off hyperp sampling)")

        ("nchartypes", po::value<U>(&data.nchartypes)->default_value(0),
         "Number of characters assumed in P_0 (default = 0 will compute from input)")

        // ("p_nl", po::value<F>(&data.p_nl)->default_value(0.5),
        // "End of sentence prob")

        ("aeos", po::value<F>(&data.aeos)->default_value(2),
         "Beta prior on end of sentence prob")

        ("init-pboundary", po::value<F>(&data.init_pboundary)->default_value(0),
         "Initial segmentation boundary probability (-1 = gold)")

        ("pya-beta-a", po::value<F>(&data.pya_beta_a)->default_value(1),
         "if non-zero, a parameter of Beta prior on pya")

        ("pya-beta-b", po::value<F>(&data.pya_beta_b)->default_value(1),
         "if non-zero, b parameter of Beta prior on pya")

        ("pya-gamma-s", po::value<F>(&data.pyb_gamma_s)->default_value(10),
         "if non-zero, parameter of Gamma prior on pyb")

        ("pya-gamma-c", po::value<F>(&data.pyb_gamma_c)->default_value(0.1),
         "if non-zero, parameter of Gamma prior on pyb")

        ("randseed", po::value<U>()->default_value(
            // this is the current time as integer
            std::chrono::system_clock::now().time_since_epoch().count()),
         "Random number seed, default is based on current time")

        ("trace-every", po::value<U>(&data.trace_every)->default_value(100),
         "Epochs between printing out trace information (0 = don't trace)")

        ("nsubjects,s", po::value<U>()->default_value(1),
         "Number of subjects to simulate")

        ("forget-rate,f", po::value<F>()->default_value(0),
         "Number of utterances whose words can be remembered")

        ("burnin-iterations,i", po::value<U>(&data.burnin_iterations)->default_value(2000),
         "Number of burn-in epochs. This is actually the total number of "
         "iterations through the training data.")

        ("anneal-iterations", po::value<U>(&data.anneal_iterations)->default_value(0),
         "Number of epochs to anneal for. So e.g. burn-in = 100 and anneal = 90 "
         "would leave 10 iters at the end at the final annealing temp.")

        ("anneal-start-temperature", po::value<F>(&data.anneal_start_temperature)->default_value(1),
         "Start annealing at this temperature")

        ("anneal-stop-temperature", po::value<F>(&data.anneal_stop_temperature)->default_value(1),
         "Stop annealing at this temperature")

        ("anneal-a", po::value<F>(&data.anneal_a)->default_value(0),
         "Parameter in annealing temperature sigmoid function (0 = use ACL06 schedule)")

        ("anneal-b", po::value<F>(&data.anneal_b)->default_value(0.2),
         "Parameter in annealing temperature sigmoid function")

        ("result-field-separator", po::value<std::string>(&csep)->default_value("\t"),
         "Field separator used to print results")

        ("forget-method", po::value< std::string>(&data.forget_method)->default_value("U"),
         "Method of deleting lexical items: U(niformly), P(roportional)")

        ("token-memory,N", po::value<U>(&data.token_memory)->default_value(0),
         "Number of tokens that can be remembered (0 = no limit)")

        ("type-memory,L", po::value<U>(&data.type_memory)->default_value(0),
         "Number of types that can be remembered (0 = no limit)")
        ;

    // parse the command line arguments and store them in vm
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // if --help given, display the help message and exit
    if (vm.count("help"))
    {
        std::cerr << desc << std::endl;
        exit(0);
    }

    // if --config-file given, parse the argument specified in that file
    if (vm.count("config-file") > 0)
    {
        std::ifstream is(vm["config-file"].as<std::string>().c_str());
        po::store(po::parse_config_file(is, desc), vm);
        po::notify(vm);
    }

    // setup data_file to stdin or file
    std::string data_file = "stdin";
    if (vm.count("data-file") > 0)
        data_file = vm["data-file"].as<std::string>();

    // setup eval file if specified in arguments
    std::string eval_file = "none";
    if (vm.count("eval-file") > 0)
        eval_file = vm["eval-file"].as<std::string>();

    // We need a wide version of the result-field-separator parameter string
    sep.assign(csep.begin(), csep.end());

    if (debug_level >= 20000)
    {
        // display detailed input arguments
        std::wcout
            << "# particle" << ", " << std::chrono::system_clock::now().time_since_epoch().count()
            << "# debug-level=" << debug_level << std::endl
            << "# data-file=" << data_file.c_str() << std::endl
            << "# data-start-index=" << vm["data-start-index"].as<U>() << std::endl
            << "# data-num-sents=" << vm["data-num-sents"].as<U>() << std::endl
            << "# eval-file=" << eval_file.c_str() << std::endl
            << "# eval-start-index=" << vm["eval-start-index"].as<U>() << std::endl
            << "# eval-num-sents=" << vm["eval-num-sents"].as<U>() << std::endl
            << "# eval-maximize=" << vm["eval-maximize"].as<U>() << std::endl
            << "# eval-interval=" <<vm["eval-interval"].as<U>() << std::endl
            << "# output-file=" << str2wstr(vm["output-file"].as<std::string>()) << std::endl
            << "# estimator=" << str2wstr(vm["estimator"].as<std::string>()) << std::endl
            << "# decay_rate=" << vm["decay-rate"].as<F>() << std::endl
            << "# samples_per_utt=" << vm["samples-per-utt"].as<U>() << std::endl
            << "# mode=" << str2wstr(vm["mode"].as<std::string>()) << std::endl
            << "# ngram=" << vm["ngram"].as<U>() << std::endl
            << "# do_mbdp=" << vm["do-mbdp"].as<bool>() << std::endl
            << "# a1=" << vm["a1"].as<F>() << std::endl
            << "# b1=" << vm["b1"].as<F>() << std::endl
            << "# a2=" << vm["a2"].as<F>() << std::endl
            << "# b2=" << vm["b2"].as<F>() << std::endl
            << "# Pstop=" << data.Pstop << std::endl
            << "# hypersamp-ratio=" << data.hypersampling_ratio << std::endl
            << "# aeos=" << data.aeos << std::endl
            << "# init_pboundary=" << vm["init-pboundary"].as<F>() << std::endl
            << "# pya-beta-a="  << data.pya_beta_a << std::endl
            << "# pya-beta-b="  << data.pya_beta_b << std::endl
            << "# pyb-gamma-s="  << data.pyb_gamma_s << std::endl
            << "# pyb-gamma-c="  << data.pyb_gamma_c << std::endl
            << "# randseed=" << vm["randseed"].as<U>() << std::endl
            << "# trace-every=" << data.trace_every << std::endl
            << "# nsubjects=" << vm["nsubjects"].as<U>() << std::endl
            << "# forget-rate=" << vm["forget-rate"].as<F>() << std::endl
            << "# burnin-iterations=" << data.burnin_iterations << std::endl
            << "# anneal-iterations=" << data.anneal_iterations << std::endl
            << "# anneal-start-temperature=" << data.anneal_start_temperature << std::endl
            << "# anneal-stop-temperature=" << data.anneal_stop_temperature << std::endl
            << "# anneal-a=" << data.anneal_a << std::endl
            << "# anneal-b=" << data.anneal_b << std::endl
            << "# result-field-separator=" << sep << std::endl;
    }

    unif01.seed(vm["randseed"].as<U>());

    // read training data
    if (data_file != "stdin")
    {
        std::wifstream is(data_file.c_str());
        if (!is)
        {
            std::cerr << "Error: couldn't open " << data_file << std::endl;
            exit(1);
        }
        // is.imbue(std::locale(std::locale(), new utf8_codecvt_facet()));

        data.read(is, vm["data-start-index"].as<U>(), vm["data-num-sents"].as<U>());
    }
    else
    {
        data.read(std::wcin, vm["data-start-index"].as<U>(), vm["data-num-sents"].as<U>());
    }

    // read evaluation data
    if (eval_file !=  "none")
    {
        std::wifstream is(eval_file.c_str());
        if (!is)
        {
            std::cerr << "Error: couldn't open " << eval_file << std::endl;
            exit(1);
        }
        // is.imbue(std::locale(std::locale(), new utf8_codecvt_facet()));
        data.read_eval(is,vm["eval-start-index"].as<U>(),vm["eval-num-sents"].as<U>());
    }

    if (debug_level >= 98000) {
        TRACE(S::data.size());
        TRACE(S::data);
        TRACE(data.sentence_boundary_list());
        TRACE(data.nchars());
        TRACE(data.possible_boundaries());
        TRACE(data.true_boundaries());
    }

    if (debug_level >= 100)
    {
        std::wcout << "# nchartypes=" << data.nchartypes << std::endl
                   << "# nsentences=" << data.nsentences() << std::endl;
    }

    // open the output file, handle UTF8
    std::wofstream os(vm["output-file"].as<std::string>().c_str());
    if (! os)
    {
        std::cerr << "couldn't open output file: "
                  << vm["output-file"].as<std::string>()
                  << std::endl;
        exit(1);
    }
    // os.imbue(utf8_locale);

    for(unsigned int subject = 0; subject < vm["nsubjects"].as<unsigned int>(); subject++)
    {
        Model* sampler = get_sampler(
            &data,
            vm["ngram"].as<unsigned int>(),
            vm["mode"].as<std::string>(),
            vm["estimator"].as<std::string>(),
            vm["forget-rate"].as<F>(),
            vm["decay-rate"].as<F>(),
            vm["samples-per-utt"].as<U>());

        std::wcout << "initial probability = " << sampler->log_posterior() << std::endl;
        assert(sampler->sanity_check());

        // if want to evaluate test set during training intervals, need to add
        // that into estimate function
        sampler->estimate(
            data.burnin_iterations, std::wcout, vm["eval-interval"].as<U>(),
            1, vm["eval-maximize"].as<U>(), true);

        // evaluates test set at the end of training
        if (eval_file == "none")
        {
            sampler->print_segmented(os);
            sampler->print_scores(std::wcout);
            std::wcout << "final posterior = " << sampler->log_posterior() << std::endl;
        }
        else
        {
            if (debug_level >= 5000)
            {
                std::wcout << "segmented training data:" << std::endl;
                sampler->print_segmented(std::wcout);
                sampler->print_scores(std::wcout);
                std::wcout << "training final posterior = " << sampler->log_posterior() << std::endl;
                std::wcout << "segmented test data:" << std::endl;
            }

            std::wcout << "Test set at end of training " << std::endl;
            sampler->run_eval(os,1,vm["eval-maximize"].as<U>());

            std::wcout << "testing final posterior = " << sampler->log_posterior() << std::endl;
            sampler->print_eval_segmented(os);
            sampler->print_eval_scores(std::wcout);
        }
        os << std::endl;
        delete sampler;
    }
}
