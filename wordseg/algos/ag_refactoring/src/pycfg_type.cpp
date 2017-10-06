#include "pycky.hh"

#include "logging.hh"


std::istream& readline_symbols(std::istream& is, Ss& syms) {
    syms.clear();
    std::string line;
    if (std::getline(is, line))
    {
        std::istringstream iss(line);
        std::string s;
        while (iss >> s)
            syms.push_back(s);
    }
    return is;
}


pycfg_type::pycfg_type()
    : estimate_theta_flag(false), predictive_parse_filter(false),
      default_weight(1), default_pya(1e-1), default_pyb(1e3),
      pya_beta_a(0), pya_beta_b(0), pyb_gamma_s(0), pyb_gamma_c(0)
{}


pycfg_type::~pycfg_type()
{}


F pycfg_type::get_pya(S parent) const
{
    S_F::const_iterator it = parent_pya.find(parent);
    return (it == parent_pya.end()) ? default_pya : it->second;
}


F pycfg_type::set_pya(S parent, F pya)
{
    F old_pya = default_pya;
    S_F::iterator it = parent_pya.find(parent);
    if (it != parent_pya.end())
        old_pya = it->second;

    if (pya != default_pya)
        parent_pya[parent] = pya;

    else // pya == default_pya
        if (it != parent_pya.end())
            parent_pya.erase(it);

    return old_pya;
}


F pycfg_type::get_pyb(S parent) const
{
    S_F::const_iterator it = parent_pyb.find(parent);
    return (it == parent_pyb.end()) ? default_pyb : it->second;
}


std::size_t pycfg_type::sum_pym() const
{
    std::size_t sum = 0;
    for (const auto& it: parent_pym)
        sum += it.second;
    return sum;
}

std::size_t pycfg_type::terms_pytrees_size() const
{
    std::size_t size = 0;
    terms_pytrees.for_each(terms_pytrees_size_helper(size));
    return size;
}


F pycfg_type::tree_prob(const tree* tp) const
{
    if (tp->children().empty())
        return 1;

    F pya = get_pya(tp->label());
    if (pya == 1)
    { // no cache
        F prob = 1;
        Ss children;
        for(const auto& it: tp->children())
        {
            children.push_back(it->label());
            prob *= tree_prob(it);
        }
        prob *= rule_prob(tp->label(), children);
        return prob;
    }
    F pyb = get_pyb(tp->label());
    std::size_t pym = dfind(parent_pym, tp->label());
    std::size_t pyn = dfind(parent_pyn, tp->label());
    if (tp->count() > 0) { // existing node
        assert(tp->count() <= pyn);
        assert(pym > 0);
        F prob = (tp->count() - pya)/(pyn + pyb);
        assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
        return prob;
    }

    // new node
    F prob = (pym * pya + pyb)/(pyn + pyb);
    assert(finite(prob)); assert(prob > 0); assert(prob <= 1);
    Ss children;
    for(const auto& it: tp->children())
    {
        children.push_back(it->label());
        prob *= tree_prob(it);
    }
    prob *= rule_prob(tp->label(), children);

    if (prob < 0)
        LOG(error) << "pycfg_type::tree_prob(" << *tp << ") = " << prob;

    assert(finite(prob));
    assert(prob <= 1);
    assert(prob >= 0);

    return prob;
}


F pycfg_type::incrtree(tree* tp, std::size_t weight)
{
    if (tp->children().empty())
        return 1;  // terminal node

    assert(weight >= 0);
    F pya = get_pya(tp->label());    // PY cache statistics
    F pyb = get_pyb(tp->label());

    if (pya == 1)
    { // don't table this category
        F prob = 1;
        Ss children;
        for (const auto& it: tp->children())
            children.push_back(it->label());

        prob *= incrrule(tp->label(), children, estimate_theta_flag*weight);

        for (const auto& it: tp->children())
            prob *= incrtree(it, weight);

        return prob;
    }

    else if (tp->count() > 0)
    {  // old PY table entry
        std::size_t& pyn = parent_pyn[tp->label()];
        F prob = (tp->count() - pya) / (pyn + pyb);

        assert(finite(prob));
        assert(prob > 0);
        assert(prob <= 1);

        // increment entry and PY counts
        tp->increment(weight);
        pyn += weight;

        return prob;
    }
    else
    { // new PY table entry
        Ss terms;
        tp->terminals(terms);
        bool inserted ATTRIBUTE_UNUSED = terms_pytrees[terms].insert(tp).second;
        assert(inserted);

        std::size_t& pym = parent_pym[tp->label()];
        std::size_t& pyn = parent_pyn[tp->label()];
        F prob = (pym*pya + pyb)/(pyn + pyb);  // select new table

        assert(finite(prob));
        assert(prob > 0);
        assert(prob <= 1);

        tp->increment(weight);              // increment count
        pym += 1;                         // one more PY table entry
        pyn += weight;                    // increment PY count

        Ss children;
        for (const auto& it: tp->children())
            children.push_back(it->label());

        prob *= incrrule(tp->label(), children, estimate_theta_flag * weight);

        for (const auto& it: tp->children())
            prob *= incrtree(it, weight);

        return prob;
    }
}


F pycfg_type::decrtree(tree* tp, std::size_t weight)
{
    if (tp->children().empty())
        return 1;  // terminal node

    // PY cache statistics
    F pya = get_pya(tp->label());

    if (pya == 1)
    {  // don't table this category
        F prob = 1;

        Ss children;
        for (const auto& it: tp->children())
            children.push_back(it->label());

        F ruleprob = decrrule(tp->label(), children, estimate_theta_flag * weight);

        assert(ruleprob > 0);
        prob *= ruleprob;

        for (const auto& it: tp->children())
            prob *= decrtree(it, weight);

        return prob;
    }

    assert(weight <= tp->count());
    tp->decrement(weight);

    assert(afind(parent_pyn, tp->label()) >= weight);
    const std::size_t pyn = (parent_pyn[tp->label()] -= weight);
    F pyb = get_pyb(tp->label());

    if (tp->count() > 0)
    {  // old PY table entry
        assert(pyn > 0);
        F prob = (tp->count() - pya) / (pyn + pyb);

        assert(finite(prob));
        assert(prob > 0);
        assert(prob <= 1);

        return prob;
    }
    else
    { // tp->count == 0, remove PY table entry
        Ss terms;
        tp->terminals(terms);
        sT& pytrees = terms_pytrees[terms];
        sT::size_type nerased ATTRIBUTE_UNUSED = pytrees.erase(tp);

        assert(nerased == 1);
        if (pytrees.empty())
            terms_pytrees.erase(terms);

        // Bug: when pym or pyn goes to zero and the parent is erased,
        // and then the reference to pym or pyn becomes a dangling reference
        // std::size_t& pym = parent_pym[tp->label()];
        // pym -= 1;                         // reduce cache count
        assert(parent_pym.count(tp->label()) > 0);
        const std::size_t pym = --parent_pym[tp->label()];

        if (pym == 0)
            parent_pym.erase(tp->label());

        if (pyn == 0)
            parent_pyn.erase(tp->label());

        F prob = (pym*pya + pyb)/(pyn + pyb);  // select new table
        assert(finite(prob));
        assert(prob > 0);
        assert(prob <= 1);

        Ss children;
        for (const auto& it: tp->children())
            children.push_back(it->label());

        prob *= decrrule(tp->label(), children, estimate_theta_flag*weight);
        assert(prob > 0);

        for (const auto& it: tp->children())
            prob *= decrtree(it, weight);

        return prob;
    }
}


std::istream& pycfg_type::read(std::istream& is)
{
    start = symbol::undefined();

    F weight = default_weight;
    F pya = default_pya;
    F pyb = default_pyb;
    S parent;
    while (is >> default_value(weight, default_weight)
           >> default_value(pya, default_pya)
           >> default_value(pyb, default_pyb)
           >> parent >> " -->")
    {
        if (weight<=0)
            weight=default_weight;

        if (start.is_undefined())
            start = parent;

        Ss rhs;
        readline_symbols(is, rhs);

        LOG(trace) << "# " << weight << '\t' << parent << " --> " << rhs;

        if (pya < 0 || pya > 1)
        {
            LOG(fatal) << "Error while reading grammar rule " << parent << " --> " << rhs
                       << ": pya = " << pya << " is out of bounds 0 <= pya <= 1. Exiting.";
            exit(1);
        }

        if (pyb <= 0)
        {
            LOG(fatal) << "Error while reading grammar rule " << parent << " --> " << rhs
                       << ": pyb = " << pyb << " is out of bounds 0 < pyb. Exiting.";
            exit(1);
        }

        incrrule(parent, rhs, weight);

        if (pya != default_pya)
            parent_pya[parent] = pya;

        if (pyb != default_pyb)
            parent_pyb[parent] = pyb;

        rule_priorweight[SSs(parent,rhs)] += weight;
        parent_priorweight[parent] += weight;
    }

    return is;
}


std::ostream& pycfg_type::write(std::ostream& os) const
{
    assert(start.is_defined());

    write_rules(os, start);
    for (const auto& it: parent_weight)
        if (it.first != start)
            write_rules(os, it.first);

    return os;
}


std::ostream& pycfg_type::write_rules(std::ostream& os, S parent) const
{
    rhs_parent_weight.for_each(write_rule(os, parent));

    for (const auto& it0: unarychild_parent_weight)
    {
        S child = it0.first;
        for (const auto& it1: it0.second)
            if (it1.first == parent)
                os << it1.second << '\t' << parent
                   << " --> " << child << std::endl;
    }

    // save the compact_trees flag
    bool old_compact_trees_flag = catcount_tree::get_compact_trees();
    catcount_tree::set_compact_trees(false);
    terms_pytrees.for_each(write_pycache(os, parent));
    catcount_tree::set_compact_trees(old_compact_trees_flag);

    return os;
}


F pycfg_type::logPcorpus() const
{
    F logP = 0;

    // grammar part
    for (const auto& it: rule_priorweight)
    {
        S parent = it.first.first;
        const Ss& rhs = it.first.second;
        F priorweight = it.second;
        F weight = rule_weight(parent, rhs);
        logP += lgamma(weight) - lgamma(priorweight);
    }

    for (const auto& it: parent_priorweight)
    {
        F weight = dfind(parent_weight, it.first);
        logP += lgamma(it.second) - lgamma(weight);
    }

    assert(logP <= 0);

    // PY adaptor part
    for (const auto& it: parent_pyn)
    {
        S parent = it.first;
        std::size_t pyn = it.second;
        std::size_t pym = afind(parent_pym, parent);
        F pya = get_pya(parent);
        F pyb = get_pyb(parent);
        logP += lgamma(pyb) - lgamma(pyn+pyb);
        for (std::size_t i = 0; i < pym; ++i)
            logP += log(i*pya + pyb);
    }

    terms_pytrees.for_each(logPcache(*this, logP));
    assert(logP <= 0);

    return logP;
}


F pycfg_type::logPrior() const
{
    F sumLogP = 0;
    if (pyb_gamma_s > 0 && pyb_gamma_c > 0)
        for (const auto& it: parent_pyn)
        {
            S parent = it.first;
            F pya = get_pya(parent);

            assert(pya >= 0);
            assert(pya <= 1);

            F pyb = get_pyb(parent);
            assert(pyb >= 0);

            if (pya_beta_a > 0 && pya_beta_b > 0 && pya > 0)
            {
                F logP = pya_logPrior(pya, pya_beta_a, pya_beta_b);
                // TRACE5(parent, logP, pya, pya_beta_a, pya_beta_b);
                sumLogP += logP;
            }
            F logP = pyb_logPrior(pyb, pyb_gamma_c, pyb_gamma_s);
            // TRACE5(parent, logP, pyb, pyb_gamma_c, pyb_gamma_s);
            sumLogP += logP;
        }

    return sumLogP;
}


F pycfg_type::pya_logPrior(F pya, F pya_beta_a, F pya_beta_b)
{
    // prior for pya
    F prior = lbetadist(pya, pya_beta_a, pya_beta_b);
    return prior;
}

F pycfg_type::pyb_logPrior(F pyb, F pyb_gamma_c, F pyb_gamma_s)
{
    // prior for pyb
    F prior = lgammadist(pyb, pyb_gamma_c, pyb_gamma_s);
    return prior;
}


struct pycfg_type::resample_pyb_type
{
    typedef double F;

    std::size_t pyn, pym;
    F pya, pyb_gamma_c, pyb_gamma_s, min_pyb;

    resample_pyb_type(std::size_t pyn, std::size_t pym, F pya, F pyb_gamma_c, F pyb_gamma_s, F min_pyb)
        : pyn(pyn), pym(pym), pya(pya), pyb_gamma_c(pyb_gamma_c), pyb_gamma_s(pyb_gamma_s), min_pyb(min_pyb)
        { }

    // operator() returns the part of the log posterior
    // probability that depends on pyb
    F operator() (F pyb0) const {
        F pyb = pyb0+min_pyb;
        assert(pyb > 0);

        F logPrior = pyb_logPrior(pyb, pyb_gamma_c, pyb_gamma_s);  //!< prior for pyb
        F logProb = 0;
        logProb += (pya == 0 ? pym*log(pyb) : pym*log(pya) + lgamma(pym + pyb/pya) - lgamma(pyb/pya));
        logProb += lgamma(pyb) - lgamma(pyn+pyb);

        return logProb + logPrior;
    }
};


struct pycfg_type::resample_pya_type
{
    std::size_t pyn, pym;
    F pyb, pya_beta_a, pya_beta_b;
    const Ts& trees;

    resample_pya_type(std::size_t pyn, std::size_t pym, F pyb, F pya_beta_a, F pya_beta_b, const Ts& trees)
        : pyn(pyn), pym(pym), pyb(pyb), pya_beta_a(pya_beta_a), pya_beta_b(pya_beta_b), trees(trees)
        {}

    // operator() returns the part of the log posterior
    // probability that depends on pya
    F operator() (F pya) const
        {
            F logPrior = pya_logPrior(pya, pya_beta_a, pya_beta_b);     //!< prior for pya
            F logProb = 0;
            F lgamma1a = lgamma(1-pya);
            for (const auto& it: trees)
            {
                std::size_t count = it->count();
                logProb += lgamma(count-pya) - lgamma1a;
            }

            logProb += (pya == 0 ? pym*log(pyb) : pym*log(pya) + lgamma(pym + pyb/pya) - lgamma(pyb/pya));

            return logPrior + logProb;
        }
};


void pycfg_type::resample_pyb()
{
    std::size_t niterations = 20;   //!< number of resampling iterations
    F min_pyb = 1e-20;    //!< minimum value for pyb
    for (const auto& it: parent_pyn)
    {
        S parent = it.first;
        std::size_t pyn = it.second;
        std::size_t pym = afind(parent_pym, parent);
        F pya = get_pya(parent);
        F pyb = get_pyb(parent);

        resample_pyb_type pyb_logP(pyn, pym, pya, pyb_gamma_c, pyb_gamma_s, min_pyb);

        F pyb0 = slice_sampler1dp(pyb_logP, pyb, random1, 1, niterations);
        parent_pyb[parent] = pyb0 + min_pyb;
    }
}


void pycfg_type::resample_pya(const S_Ts& parent_trees)
{
    std::size_t niterations = 20;   //!< number of resampling iterations

    for (const auto& it: parent_pyn)
    {
        S parent = it.first;
        F pya = get_pya(parent);

        if (pya == 0)   // if this nonterminal has pya == 0, then don't resample
            continue;

        F pyb = get_pyb(parent);
        std::size_t pyn = it.second;
        std::size_t pym = afind(parent_pym, parent);
        const Ts& trees = afind(parent_trees, parent);

        resample_pya_type pya_logP(pyn, pym, pyb, pya_beta_a, pya_beta_b, trees);
        pya = slice_sampler1d(pya_logP, pya, random1, std::numeric_limits<double>::min(), 1.0, 0.0, niterations);
        parent_pya[parent] = pya;
    }
}


void pycfg_type::resample_pyab()
{
    const std::size_t niterations = 5;  //!< number of alternating samples of pya and pyb
    S_Ts parent_trees;
    terms_pytrees.for_each(resample_pyab_parent_trees_helper(parent_trees));
    for (std::size_t i=0; i<niterations; ++i)
    {
        resample_pyb();
        resample_pya(parent_trees);
    }

    resample_pyb();
}


std::ostream& pycfg_type::write_adaptor_parameters(std::ostream& os) const
{
    for (const auto& it: parent_priorweight)
    {
        S parent = it.first;
        F pya = get_pya(parent);

        if (pya == 1)
            continue;

        std::size_t pym = dfind(parent_pym, parent);
        std::size_t pyn = dfind(parent_pyn, parent);
        F pyb = get_pyb(parent);
        os << ' ' << parent << ' ' << pym << ' ' << pyn << ' ' << pya << ' ' << pyb;
    }

    return os;
}


void pycfg_type::initialize_predictive_parse_filter()
{
    predictive_parse_filter = true;
    for (const auto& it: rule_priorweight)
    {
        const auto& rule = it.first;
        const auto& children = rule.second;
        assert(!children.empty());

        S child1 = children.front();
        predictive_parse_filter_grammar.add_rule(
            it.first, children.size() == 1 && !parent_priorweight.count(child1));
    }
}


std::istream& operator>> (std::istream& is, pycfg_type& g)
{
    return g.read(is);
}


std::ostream& operator<< (std::ostream& os, const pycfg_type& g)
{
    return g.write(os);
}
