#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

// with gcc below the version 5.0, the <codecvt> header of the
// standard library is not implemented, so we use the codecvt in
// boost::locale instead
#define BOOST_UTF8_BEGIN_NAMESPACE
#define BOOST_UTF8_END_NAMESPACE
#define BOOST_UTF8_DECL
#include <boost/detail/utf8_codecvt_facet.ipp>

#include "text/corpus_data.hh"
#include "estimator/parameters.hh"
#include "estimator/factory.hh"
#include "random-mt19937ar.hpp"


// TODO The following are global variables accessed across the whole
// code (with 'extern' declarations). This is bad (source of bugs,
// hard to read/debug). Use parameters instead.
uniform01_type unif01;    // random number generator
std::size_t debug_level;  // higher -> mode debug messages on stdout
std::wstring sep;         // separator used to separate fields during printing of results


std::wstring str2wstr(std::string str)
{
    std::wstring temp_str(str.length(), L' ');  // Make room for characters
    std::copy(str.begin(), str.end(), std::back_inserter(temp_str));
    return temp_str;
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

    estimator::parameters params;
    text::corpus_data corpus;
    std::string csep;

    // define the command line arguments
    namespace po = boost::program_options;
    po::options_description desc("program_options");
    desc.add_options()
        ("help,h",
         "produce help message")

        ("config-file,C", po::value<std::string>(),
         "read options from this file")

        ("debug-level,d", po::value<std::size_t>(&debug_level)->default_value(0),
         "debugging level")

        ("data-file", po::value<std::string>(),
         "training data file (default is stdin)")

        ("data-start-index", po::value<std::size_t>()->default_value(0),
         "sentence index to start reading training data file")

        ("data-num-sents", po::value<std::size_t>()->default_value(0),
         "number of training sentences to use (0 = all)")

        ("eval-file", po::value<std::string>(),
         "testing data file (default is training file). "
         "If no eval-file is listed, evaluation will be on the training file. "
         "Note that listing the same file for both training and testing has "
         "different functionality than not listing a test file, due to the way "
         "that the test file is segmented.")

        ("eval-start-index", po::value<std::size_t>()->default_value(0),
         "sentence index to start reading eval data file")

        ("eval-num-sents", po::value<std::size_t>()->default_value(0),
         "number of testing sentences to use (0 = all)")

        ("eval-maximize", po::value<std::size_t>()->default_value(0),
         "1 = choose max prob segmentation of test sentences, 0 (default) = sample instead")

        ("eval-interval", po::value<std::size_t>()->default_value(0),
         "how many iterations are run before the test set is evaluated, 0 "
         "(default) means to only evaluate the test set after all iterations "
         "are complete.w")

        ("output-file,o", po::value<std::string>()->required(),
         "segmented output file")

        ("estimator", po::value<std::string>()->default_value("F"),
         "possible values are: V(iterbi), F(lip), T(ree), D(ecayed Flip). "
         "Viterbi does dynamic programming maximization, Tree does dynamic "
         "programming sampling, Flip does original Gibbs sampler.")

        ("decay-rate", po::value<double>()->default_value(1.0),
         "decay rate for D(ecayed Flip), default = 1.0")

        ("samples-per-utt", po::value<std::size_t>()->default_value(1000),
         "samples per utterance for D(ecayed Flip), default = 1000")

        ("mode", po::value<std::string>()->default_value("batch"),
         "possible values are: online, batch")

        ("ngram", po::value<std::size_t>()->default_value(2),
         "possible values are: 1 (unigram), 2 (bigram)")

        ("do-mbdp", po::value<bool>(&params.do_mbdp)->default_value(false),
         "maximize using Brent ngram instead of DP")

        ("a1", po::value<double>(&params.a1)->default_value(0),
         "Unigram Pitman-Yor a parameter")

        ("b1", po::value<double>(&params.b1)->default_value(1),
         "Unigram Pitman-Yor b parameter")

        ("a2", po::value<double>(&params.a2)->default_value(0),
         "Bigram Pitman-Yor a parameter")

        ("b2", po::value<double>(&params.b2)->default_value(1),
         "Bigram Pitman-Yor b parameter")

        ("Pstop", po::value<double>(&params.pstop)->default_value(0.5),
         "Monkey model stop probability")

        // not completely sure that hyper parameter sampling is
        // working correctly yet. However it does yield pretty good
        // results when using all four PY parameters.
        ("hypersamp-ratio", po::value<double>(&params.hypersampling_ratio)->default_value(0.1),
         "Standard deviation for new hyperparm proposals (0 turns off hyperp sampling)")

        // ("nchartypes", po::value<std::size_t>(&data.m_nchartypes)->default_value(0),
        //  "Number of characters assumed in P_0 (default = 0 will compute from input)")

        // ("p_nl", po::value<double>(&data.p_nl)->default_value(0.5),
        // "End of sentence prob")

        ("aeos", po::value<double>(&params.aeos)->default_value(2),
         "Beta prior on end of sentence prob")

        ("init-pboundary", po::value<double>(&params.init_pboundary)->default_value(0),
         "Initial segmentation boundary probability (-1 = gold)")

        ("pya-beta-a", po::value<double>(&params.pya_beta_a)->default_value(1),
         "if non-zero, a parameter of Beta prior on pya")

        ("pya-beta-b", po::value<double>(&params.pya_beta_b)->default_value(1),
         "if non-zero, b parameter of Beta prior on pya")

        ("pya-gamma-s", po::value<double>(&params.pyb_gamma_s)->default_value(10),
         "if non-zero, parameter of Gamma prior on pyb")

        ("pya-gamma-c", po::value<double>(&params.pyb_gamma_c)->default_value(0.1),
         "if non-zero, parameter of Gamma prior on pyb")

        ("randseed", po::value<std::size_t>()->default_value(
            // this is the current time as integer
            std::chrono::system_clock::now().time_since_epoch().count()),
         "Random number seed, default is based on current time")

        ("trace-every", po::value<std::size_t>(&params.trace_every)->default_value(100),
         "Epochs between printing out trace information (0 = don't trace)")

        ("nsubjects,s", po::value<std::size_t>()->default_value(1),
         "Number of subjects to simulate")

        ("forget-rate,f", po::value<double>()->default_value(0),
         "Number of utterances whose words can be remembered")

        ("burnin-iterations,i", po::value<std::size_t>()->default_value(2000),
         "Number of burn-in epochs. This is actually the total number of "
         "iterations through the training data.")

        ("anneal-iterations", po::value<std::size_t>()->default_value(0),
         "Number of epochs to anneal for. So e.g. burn-in = 100 and anneal = 90 "
         "would leave 10 iters at the end at the final annealing temp.")

        ("anneal-start-temperature", po::value<double>()->default_value(1),
         "Start annealing at this temperature")

        ("anneal-stop-temperature", po::value<double>()->default_value(1),
         "Stop annealing at this temperature")

        ("anneal-a", po::value<double>()->default_value(0),
         "Parameter in annealing temperature sigmoid function (0 = use ACL06 schedule)")

        ("anneal-b", po::value<double>()->default_value(0.2),
         "Parameter in annealing temperature sigmoid function")

        ("result-field-separator", po::value<std::string>(&csep)->default_value("\t"),
         "Field separator used to print results")

        ("forget-method", po::value< std::string>(&params.forget_method)->default_value("U"),
         "Method of deleting lexical items: U(niformly), P(roportional)")

        ("token-memory,N", po::value<std::size_t>(&params.token_memory)->default_value(0),
         "Number of tokens that can be remembered (0 = no limit)")

        ("type-memory,L", po::value<std::size_t>(&params.type_memory)->default_value(0),
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
            << "# debug-level=" << debug_level << std::endl
            << "# data-file=" << data_file.c_str() << std::endl
            << "# data-start-index=" << vm["data-start-index"].as<std::size_t>() << std::endl
            << "# data-num-sents=" << vm["data-num-sents"].as<std::size_t>() << std::endl
            << "# eval-file=" << eval_file.c_str() << std::endl
            << "# eval-start-index=" << vm["eval-start-index"].as<std::size_t>() << std::endl
            << "# eval-num-sents=" << vm["eval-num-sents"].as<std::size_t>() << std::endl
            << "# eval-maximize=" << vm["eval-maximize"].as<std::size_t>() << std::endl
            << "# eval-interval=" <<vm["eval-interval"].as<std::size_t>() << std::endl
            << "# output-file=" << str2wstr(vm["output-file"].as<std::string>()) << std::endl
            << "# estimator=" << str2wstr(vm["estimator"].as<std::string>()) << std::endl
            << "# decay_rate=" << vm["decay-rate"].as<double>() << std::endl
            << "# samples_per_utt=" << vm["samples-per-utt"].as<std::size_t>() << std::endl
            << "# mode=" << str2wstr(vm["mode"].as<std::string>()) << std::endl
            << "# ngram=" << vm["ngram"].as<std::size_t>() << std::endl
            << "# do_mbdp=" << vm["do-mbdp"].as<bool>() << std::endl
            << "# a1=" << vm["a1"].as<double>() << std::endl
            << "# b1=" << vm["b1"].as<double>() << std::endl
            << "# a2=" << vm["a2"].as<double>() << std::endl
            << "# b2=" << vm["b2"].as<double>() << std::endl
            << "# Pstop=" << params.pstop << std::endl
            << "# hypersamp-ratio=" << params.hypersampling_ratio << std::endl
            << "# aeos=" << params.aeos << std::endl
            << "# init_pboundary=" << vm["init-pboundary"].as<double>() << std::endl
            << "# pya-beta-a="  << params.pya_beta_a << std::endl
            << "# pya-beta-b="  << params.pya_beta_b << std::endl
            << "# pyb-gamma-s="  << params.pyb_gamma_s << std::endl
            << "# pyb-gamma-c="  << params.pyb_gamma_c << std::endl
            << "# randseed=" << vm["randseed"].as<std::size_t>() << std::endl
            << "# trace-every=" << params.trace_every << std::endl
            << "# nsubjects=" << vm["nsubjects"].as<std::size_t>() << std::endl
            << "# forget-rate=" << vm["forget-rate"].as<double>() << std::endl
            << "# burnin-iterations=" << vm["burnin-iterations"].as<std::size_t>() << std::endl
            << "# anneal-iterations=" << vm["anneal-iterations"].as<std::size_t>() << std::endl
            << "# anneal-start-temperature=" << vm["anneal-start-temperature"].as<double>() << std::endl
            << "# anneal-stop-temperature=" << vm["anneal-stop-temperature"].as<double>() << std::endl
            << "# anneal-a=" << vm["anneal_a"].as<double>() << std::endl
            << "# anneal-b=" << vm["anneal-b"].as<double>() << std::endl
            << "# result-field-separator=" << sep << std::endl;
    }

    unif01.seed(vm["randseed"].as<std::size_t>());

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

        corpus.read(is, vm["data-start-index"].as<std::size_t>(), vm["data-num-sents"].as<std::size_t>());
    }
    else
    {
        corpus.read(std::wcin, vm["data-start-index"].as<std::size_t>(), vm["data-num-sents"].as<std::size_t>());
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
        corpus.read_eval(is,vm["eval-start-index"].as<std::size_t>(),vm["eval-num-sents"].as<std::size_t>());
    }

    if (debug_level >= 98000)
    {
        TRACE(substring::data.size());
        TRACE(substring::data);
        TRACE(corpus.sentence_boundary_list());
        TRACE(corpus.nchars());
        TRACE(corpus.possible_boundaries());
        TRACE(corpus.true_boundaries());
    }

    if (debug_level >= 100)
    {
        std::wcout << "# nchartypes=" << corpus.nchartypes() << std::endl
                   << "# nsentences=" << corpus.nsentences() << std::endl;
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

    annealing anneal(
        vm["anneal-iterations"].as<std::size_t>(),
        vm["anneal-start-temperature"].as<double>(),
        vm["anneal-stop-temperature"].as<double>(),
        vm["anneal-a"].as<double>(),
        vm["anneal-b"].as<double>());

    for(std::size_t subject = 0; subject < vm["nsubjects"].as<std::size_t>(); subject++)
    {
        auto sampler = estimator::get_estimator(
            params, corpus, anneal,
            vm["ngram"].as<std::size_t>(),
            vm["mode"].as<std::string>(),
            vm["estimator"].as<std::string>(),
            vm["forget-rate"].as<double>(),
            vm["decay-rate"].as<double>(),
            vm["samples-per-utt"].as<std::size_t>());

        std::wcout << "initial probability = " << sampler->log_posterior() << std::endl;
        assert(sampler->sanity_check());

        // Train the sampler. If want to evaluate test set during
        // training intervals, need to add that into estimate function
        sampler->estimate(
            vm["burnin-iterations"].as<std::size_t>(), std::wcout, vm["eval-interval"].as<std::size_t>(),
            1, vm["eval-maximize"].as<std::size_t>(), true);

        std::wcerr << "training done" << std::endl;

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
            sampler->run_eval(os,1,vm["eval-maximize"].as<std::size_t>());

            std::wcout << "testing final posterior = " << sampler->log_posterior() << std::endl;
            sampler->print_eval_segmented(os);
            sampler->print_eval_scores(std::wcout);
        }

        os << std::endl;
    }
}
