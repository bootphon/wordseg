#ifndef _UNIGRAMS_H_
#define _UNIGRAMS_H_

#include <iostream>
#include "pitman_yor/adaptor.hpp"


namespace pitman_yor
{
    template <typename Base>
    class unigrams: public adaptor<Base>
    {
    private:
        typedef adaptor<Base> parent;

    public:
        unigrams(Base& base, uniform01_type& u01, double a=0, double b=1)
            : parent(base, u01, a, b)
            {}

        const std::unordered_map<typename Base::argument_type, restaurant>& types()
            {
                return parent::label_tables;
            }

        std::wostream& print(std::wostream& os) const
            {
                os << "types = " << parent::ntypes()
                   << ", tokens = " << parent::ntokens()
                   << std::endl;

                wchar_t sep = '(';
                for(const auto& item: parent::label_tables)
                {
                    os << sep << item.first << ' ';
                    // item.second.n;
                    sep = ',';
                }

                return os << "))";
            }
    };
}

#endif  // _UNIGRAMS_H_
