#ifndef PARSER_H
#define PARSER_H

#include "ncprogramblock.h"

#include <parsertl/generator.hpp>
#include <parsertl/rules.hpp>
#include <parsertl/token.hpp>
#include <parsertl/state_machine.hpp>
#include <lexertl/rules.hpp>
#include <lexertl/state_machine.hpp>

#include <vector>
#include <any>
#include <map>
//#include <functional>

struct ParserContext;

class Parser
{
    using semanticAction = void (*)(ParserContext&); //std::function<void(TData&)>;

    lexertl::rules m_lrules{lexertl::icase|lexertl::dot_not_cr_lf};
    lexertl::state_machine m_lsm;
    parsertl::token<lexertl::citerator>::token_vector m_productions;
    parsertl::rules m_grules;
    parsertl::state_machine m_gsm;
    std::map<std::size_t, semanticAction> m_semanticActionMap;
    std::vector<std::any> m_stack;
    int nestingLevel {0};

    parsertl::rules::string_vector symbols_;

public:
    Parser();
    Parser(Parser&) = delete;
    Parser(Parser&&) = delete;
    Parser& operator=(Parser&) = delete;
    Parser& operator=(Parser&&) = delete;
    ~Parser() = default;

    void reset() noexcept;
    NCProgramBlock parse(const std::string& block);

    const char* skipWS(const char* start, const char* end) const noexcept;
    std::pair<std::optional<int>, const char*> readSkipLevel(const char* start, const char* end) const;
    std::pair<std::optional<BlockNumber>, const char*> readBlockNumber(const char* start, const char* end) const;
    std::pair<std::optional<std::string>, const char*> readLabel(const char* start, const char* end) const;

    template <typename T>
    static std::size_t findCommentStartPos(const typename T::const_iterator& begin,
                                           const typename T::const_iterator& end) noexcept
    {
        bool inside_double_quot {false};
        bool inside_single_quot {false};

        typename T::const_iterator semicolonPos {begin};
        typename T::const_iterator it {begin};
        while (it < end)
        {
            if (*it == '\'')
            {
                if (inside_double_quot &&
                    it + 2 < end &&
                    *(it + 1) == '\"' &&
                    *(it + 2) == '\'')
                {
                    // "...'"'..." case, skip '"'
                    it += 3;
                    continue;
                }
                if (inside_double_quot)
                {
                    inside_single_quot = !inside_single_quot;
                }
            }
            else if (*it == '\"')
            {
                inside_double_quot = !inside_double_quot;
                semicolonPos = begin;
            }
            else if (*it == ';')
            {
                if (inside_double_quot)
                {
                    // remember this pos in case of unclosed "
                    semicolonPos = it;
                }
                else
                {
                    return it - begin;
                }
            }
            it++;
        }
        return semicolonPos != begin ? semicolonPos - begin : it - begin;
    }
};

#endif // PARSER_H
