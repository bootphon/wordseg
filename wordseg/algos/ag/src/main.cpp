//  Copyright 2009-2014 Mark Johnson
//
// Runs a Pitman-Yor process for each nonterminal to estimate an
// Adaptor Grammar

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <boost/program_options.hpp>

#include "logging.hh"
#include "pstream.hh"
#include "gibbs.hh"
#include "pycky.hh"


typedef pstream::ostream* Postreamp;
typedef std::vector<Postreamp> Postreamps;

int debug = 0;


int main(int argc, char** argv)
{
    logging::init();

    pycfg_type g;

    bool hastings_correction = true;
    bool random_order = true;
    bool delayed_initialization = false;
    bool predictive_parse_filter = false;

    unsigned niterations = 100;

    F anneal_start = 1;
    F anneal_stop = 1;
    unsigned anneal_its = 100;
    unsigned resample_pycache_nits = 0;
    F z_temp = 1;
    unsigned z_its = 0;
    unsigned long rand_init = 0;

    std::string parses_filename = "", grammar_filename = "";
    std::string test1_filename = "", test2_filename = "";
    std::vector<std::string> evalcmdstrs, test1cmdstrs, test2cmdstrs, grammarcmdstrs;
    Postreamps evalcmds, test1cmds, test2cmds, grammarcmds;
    unsigned eval_every = 1;

    unsigned nparses_iterations = 1;
    F train_frac = 1.0;
    bool train_frac_randomise = false;

    // define the command line arguments
    namespace po = boost::program_options;
    po::options_description desc("program options");
    desc.add_options()
        ("help,h",
         "produce help message")

        ("grammar-file,g", po::value<std::string>(),
         "read the grammar from this file")

        ("config-file,c", po::value<std::string>(),
         "read options from this file")

        ("log-level,d", po::value<std::string>()->default_value("warning")->implicit_value("info"),
         "log level, choose in {fatal, error, warning, info, debug, trace}")

        ("print-analysis,A", po::value<std::string>(&parses_filename),
         "print analyses of training data at termination to a file")

        ("print-last-nanalysis,N", po::value<unsigned>(&nparses_iterations),
         "print analyses during last nanal-its iterations")

        ("print-compact-trees,C", po::value<bool>()->implicit_value(true),
         "print compact trees omitting uncached categories")

        ("delay-init,D", po::value<bool>(&delayed_initialization)->implicit_value(true),
         "delay grammar initialization until all sentences are parsed")

        ("dirichlet-prior,E", po::value<bool>(&g.estimate_theta_flag)->implicit_value(true),
         "estimate rule prob (theta) using Dirichlet prior")

        ("print-grammar-file,G", po::value<std::string>(&grammar_filename),
         "print grammar at termination to a file")

        ("skip-hastings,H", po::value<bool>(&hastings_correction)->implicit_value(false),
         "skip Hastings correction of tree probabilities")

        ("ordered-parse,I", po::value<bool>(&random_order)->implicit_value(false),
         "parse sentences in order (default is random order)")

        ("predictive-parse-filter,P", po::value<bool>(&predictive_parse_filter)->implicit_value(true),
         "use a predictive Earley parse to filter useless categories")

        ("resample-pycache-niter,R", po::value<unsigned>(&resample_pycache_nits),
         "resample PY cache strings during first n iterations (-1 = forever)")

        ("niterations,n", po::value<unsigned>(&niterations),
         "number of iterations")

        ("random-seed,r", po::value<unsigned long>(&rand_init),
         "initializer for random number generator (integer)")

        ("py-a,a", po::value<F>(&g.default_pya),
         "default PY a parameter")

        ("py-b,b", po::value<F>(&g.default_pyb),
         "default PY b parameter")

        ("pya-beta-a,e", po::value<F>(&g.pya_beta_a),
         "if positive, parameter of Beta prior on pya; if negative, number of iterations to anneal pya")

        ("pya-beta-b,f", po::value<F>(&g.pya_beta_b),
         "if positive, parameter of Beta prior on pya")

        ("pyb-gamma-s,s", po::value<F>(&g.pyb_gamma_s),
         "if non-zero, parameter of Gamma prior on pyb")

        ("pyb-gamma-c,i", po::value<F>(&g.pyb_gamma_c),
         "parameter of Gamma prior on pyb")

        ("weight,w", po::value<F>(&g.default_weight),
         "default value of theta (or Dirichlet prior) in generator")

        ("train-sentences,W", po::value<F>(&train_frac),
         "train only on train_frac percentage of training sentences (ignore remainder)")

        ("random-training-fraction,S", po::value<bool>(&train_frac_randomise),
         "randomise training fraction of sentences (default: training fraction is at front)")

        ("temp-start,T", po::value<F>(&anneal_start),
         "start annealing with this temperature")

        ("temp-stop,t", po::value<F>(&anneal_stop),
         "stop annealing with this temperature")

        ("anneal-iterations,m", po::value<unsigned>(&anneal_its),
         "anneal for this many iterations")

        ("ztemp,Z", po::value<F>(&z_temp),
         "temperature used just before stopping")

        ("zits,z", po::value<unsigned>(&z_its),
         "perform zits iterations at temperature ztemp at end of run")

        ("eval-parses-cmd,X", po::value<std::vector<std::string> >(&evalcmdstrs),
         "pipe each run's parses into this command (empty line separates runs)")

        ("eval-grammar-cmd,Y", po::value<std::vector<std::string> >(&grammarcmdstrs),
         "pipe each run's grammar-rules into this command (empty line separates runs)")

        ("eval-every,x", po::value<unsigned>(&eval_every),
         "pipe trees into the eval-parses-cmd every eval-every iterations")

        ("test1-file,u", po::value<std::string>(&test1_filename),
         "test strings to be parsed (but not trained on) every eval-every iterations")

        ("test1-eval,unsigned", po::value<std::vector<std::string> >(&test1cmdstrs),
         "parses of test1-file are piped into this command")

        ("test2-file,v", po::value<std::string>(&test2_filename),
         "test strings to be parsed (but not trained on) every eval-every iterations")

        ("test2-eval,V", po::value<std::vector<std::string> >(&test2cmdstrs),
         "parses of test2-file are piped into this command")
        ;

    // parse the command line arguments and store them in vm
    po::variables_map vm;
    try
    {
        po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
        po::notify(vm);

        // TODO better if we trigger that directly in the argument parser...
        anneal_start = 1.0 / anneal_start;
        anneal_stop = 1.0 / anneal_stop;
        for (const auto& cmd : evalcmdstrs)
            evalcmds.push_back(new pstream::ostream(cmd));
        for (const auto& cmd : grammarcmdstrs)
            grammarcmds.push_back(new pstream::ostream(cmd));
        for (const auto& cmd : test1cmdstrs)
            test1cmds.push_back(new pstream::ostream(cmd));
        for (const auto& cmd : test2cmdstrs)
            test2cmds.push_back(new pstream::ostream(cmd));

        // if --help given, display the help message and exit
        if (vm.count("help"))
        {
            std::cerr << desc << std::endl;
            exit(0);
        }

        // set the logger at the requested severity level
        if (vm.count("log-level"))
            logging::set_level(vm["log-level"].as<std::string>());

        // if --config-file given, parse the argument specified in that file
        if (vm.count("config-file") > 0)
        {
            const std::string config = vm["config-file"].as<std::string>();
            LOG(info) << "loading configuration from '" << config << "'";

            std::ifstream is(config.c_str());
            if (not is)
            {
                LOG(fatal) << "cannot open configuration file '" << config << "', exiting";
                exit(1);
            }

            po::store(po::parse_config_file(is, desc), vm);
            po::notify(vm);
        }

        // read the grammar file
        if (vm.count("grammar-file") == 0)
        {
            LOG(fatal) << "grammar-file not specified, exiting";
            exit(1);
        }
        const std::string grammar_file = vm["grammar-file"].as<std::string>();
        LOG(info) << "loading grammar from '" << grammar_file << "'";
        std::ifstream is(grammar_file.c_str());
        if (not is)
        {
            LOG(fatal) << "cannot open grammar file '" << grammar_file << "', exiting";
            exit(1);
        }
        is >> g;

        // setup compact trees display if asked
        if (vm.count("print-compact-trees"))
        {
            bool compact_trees = vm["print-compact-trees"].as<bool>();
            catcount_tree::set_compact_trees(compact_trees);
            if (compact_trees)
                LOG(info) << "compact tree display enabled";
            else
                LOG(info) << "compact tree display disabled";
        }
    }
    catch(po::error& e)
    {
        LOG(fatal) << e.what() << ", exiting";
        exit(1);
    }
    LOG(debug) << vm.size() << " argument parsed from command line";

    // log the eval commands if there are ones
    if (evalcmds.size())
    {
        std::stringstream eval_cmds;
        for (const auto& cmd : evalcmdstrs)
            eval_cmds << cmd << ", ";
        std::string eval_cmds_str = eval_cmds.str();

        // remove trailing ", "
        eval_cmds_str.erase(eval_cmds_str.size() - 2);
        LOG(info) << "eval commands = " << eval_cmds_str;
    }

    // read train symbols from stdin
    std::vector<std::vector<symbol> > trains;
    std::vector<symbol> terminals;
    while (readline_symbols(std::cin, terminals))
        if (terminals.empty())
            LOG(error) << "training data sentence " << trains.size() + 1 << " is empty";
        else
            trains.push_back(terminals);
    LOG(info) << "read " << trains.size() << " lines for training";

    if (predictive_parse_filter)
        g.initialize_predictive_parse_filter();

    std::vector<std::vector<symbol> > test1s;
    if (!test1_filename.empty())
    {
        std::ifstream in1(test1_filename.c_str());
        Ss terminals;
        while (readline_symbols(in1, terminals))
            if (terminals.empty())
                std::cerr << "## Error in " << test1_filename << ": sentence "
                          << test1s.size()+1 << " is empty"
                          << std::endl;
            else
                test1s.push_back(terminals);

        if (debug >= 1000)
            std::cerr << "# test1s.size() = " << test1s.size() << std::endl;
    }

    std::vector<std::vector<symbol> > test2s;
    if (!test2_filename.empty()) {
        std::ifstream in2(test2_filename.c_str());
        Ss terminals;
        while (readline_symbols(in2, terminals))
            if (terminals.empty())
                std::cerr << "## Error in " << test2_filename << ": sentence "
                          << test2s.size()+1 << " is empty"
                          << std::endl;
            else
                test2s.push_back(terminals);

        if (debug >= 1000)
            std::cerr << "# test2s.size() = " << test2s.size() << std::endl;
    }

    if (rand_init == 0)
        rand_init = time(NULL);

    mt_init_genrand(rand_init);
    LOG(trace) << "D = " << delayed_initialization
               << ", E = " << g.estimate_theta_flag
               << ", I = " << random_order
               << ", P = " << predictive_parse_filter
               << ", R = " << resample_pycache_nits
               << ", n = " << niterations
               << ", N = " << nparses_iterations
               << ", P = " << predictive_parse_filter
               << ", w = " << g.default_weight
               << ", a = " << g.default_pya
               << ", b = " << g.default_pyb
               << ", e = " << g.pya_beta_a
               << ", f = " << g.pya_beta_b
               << ", g = " << g.pyb_gamma_s
               << ", h = " << g.pyb_gamma_c
               << ", r = " << rand_init
               << ", s = " << train_frac
               << ", S = " << train_frac_randomise
               << ", x = " << eval_every
               << ", m = " << anneal_its
               << ", Z = " << z_temp
               << ", z = " << z_its
               << ", T = " << 1.0/anneal_start
               << ", t = " << anneal_stop;

    if (train_frac < 0 || train_frac > 1)
    {
        LOG(fatal) << "-s train_frac must be between 0 and 1 but it is " << train_frac <<", exiting";
        exit(1);
    }

    std::ostream* finalparses_stream_ptr = NULL;
    if (!parses_filename.empty())
        finalparses_stream_ptr = new std::ofstream(parses_filename.c_str());

    std::ostream* grammar_stream_ptr = NULL;
    if (!grammar_filename.empty())
        grammar_stream_ptr = new std::ofstream(grammar_filename.c_str());

    // LOG(debug) << "initial grammar = " << g;
    pycky parser(g);
    LOG(info) << "initial grammar parsed";
    gibbs_estimate(g, trains, train_frac, train_frac_randomise, evalcmds, eval_every,
                   niterations, anneal_start, anneal_stop, anneal_its, z_temp, z_its,
                   hastings_correction, random_order, delayed_initialization,
                   resample_pycache_nits, nparses_iterations,
                   finalparses_stream_ptr, grammar_stream_ptr,
                   test1s, test1cmds, test2s, test2cmds, grammarcmds);

    if (finalparses_stream_ptr)
        delete finalparses_stream_ptr;

    if (grammar_stream_ptr)
        delete grammar_stream_ptr;

    for (auto cmds: {grammarcmds, evalcmds, test1cmds, test2cmds})
        for(auto it: cmds)
            delete it;

    return 0;
}
