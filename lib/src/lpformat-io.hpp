/* Copyright (C) 2016 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 *f "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ORG_VLEPROJECT_LP_LPFORMAT_IO_HPP
#define ORG_VLEPROJECT_LP_LPFORMAT_IO_HPP

#include <lpcore>
#include <deque>
#include <limits>
#include <ostream>

namespace lp { namespace details {

inline bool
iequals(const std::string& lhs, const std::string& rhs) noexcept
{
    auto sz = lhs.size();

    if (rhs.size() != sz)
        return false;

    for (std::string::size_type i = 0; i < sz; ++i)
        if (std::tolower(lhs[i]) != std::tolower(rhs[i]))
            return false;

    return true;
}

inline bool
is_operator(int c) noexcept
{
    return c == '<' or c == '>' or c == '=';
}

inline bool
is_valid_character(int c) noexcept
{
    if (std::isalnum(c))
        return true;

    switch (c) {
    case '!':
    case '"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '(':
    case ')':
    case ',':
    case '.':
    case ';':
    case '?':
    case '@':
    case '_':
    case '{':
    case '}':
    case '~':
        return true;
    default:
        return false;
    }
}

struct parser_stack
{
    parser_stack(std::istream& is_)
        : m_is(is_)
        , m_line(0)
        , m_column(0)
        , m_eof_reached(is_.eof())
    {
    }

    int peek()
    {
        if (stack.empty())
            fill();

        if (stack.empty())
            return EOF;

        return stack[0][0];
    }

    std::string top()
    {
        if (stack.empty())
            fill();

        if (stack.empty())
            throw file_format_error(file_format_error::tag::end_of_file,
                                    m_line, m_column);

        return stack.front();
    }

    std::string pop()
    {
        if (stack.empty())
            fill();

        if (stack.empty())
            throw file_format_error(file_format_error::tag::end_of_file,
                                    m_line, m_column);

        std::string ret = stack.front();
        std::tie(m_line, m_column) = m_position_stack.front();
        stack.pop_front();
        m_position_stack.pop_front();

        return ret;
    }

    bool
    is_topic()
    {
        auto str = top();

        if (iequals(str, "binary") or iequals(str, "binaries"))
            return true;

        if (iequals(str, "general"))
            return true;

        if (iequals(str, "end"))
            return true;

        if (iequals(str, "st"))
            return true;

        if (stack.size() > 1 and (iequals(str, "subject")
                                  and iequals(stack[1], "to")))
            return true;

        return false;
    }

    inline bool
    is_subject_to()
    {
        if (stack.size() <= 1) {
            if (iequals(stack[0], "st")) {
                pop();
                return true;
            }

            fill();
        }

        if (stack.size() <= 1)
            return false;

        if (iequals(stack[0], "subject") and iequals(stack[1], "to")) {
            pop();
            pop();
            return true;
        }

        return false;
    }

    inline bool
    is_bounds()
    {
        if (stack.empty())
            fill();

        if (stack.size() <= 0)
            return false;

        if (iequals(stack[0], "bounds")) {
            pop();
            return true;
        }

        return false;
    }

    inline bool
    is_binary()
    {
        if (stack.empty())
            fill();

        if (stack.size() <= 0)
            return false;

        if (iequals(stack[0], "binary") or iequals(stack[0], "binaries")) {
            pop();
            return true;
        }

        return false;
    }

    inline bool
    is_general()
    {
        if (stack.empty())
            fill();

        if (stack.size() <= 0)
            return false;

        if (iequals(stack[0], "general")) {
            pop();
            return true;
        }

        return false;
    }

    inline bool
    is_end()
    {
        if (stack.empty())
            fill();

        if (stack.size() <= 0)
            return false;

        if (iequals(stack[0], "end")) {
            pop();
            return true;
        }

        return false;
    }

    void push_front(std::string str)
    {
        m_position_stack.emplace_front(
            m_line,
            m_column + (m_column - str.length()));

        stack.push_front(std::move(str));
    }

    void substr_front(std::string::size_type i)
    {
        if (stack[0].size() > i) {
            m_position_stack.emplace_front(m_line, m_column - i);
            stack[0] = stack[0].substr(i, std::string::npos);
        } else {
            m_position_stack.pop_front();
            stack.pop_front();
        }
    }

    std::deque<std::string> stack;

    bool empty() const { return stack.empty(); }
    int line() const { return m_line; }
    int column() const { return m_column; }

private:
    std::deque<std::tuple<int, int>> m_position_stack;
    std::istream& m_is;
    int m_line;
    int m_column;
    bool m_eof_reached;

    void fill()
    {
        std::string line;

        do {
            std::getline(m_is, line);
            m_line++;

            if (line.empty() and m_is.eof())
                return;

            std::string::size_type i {0};
            std::string::size_type e {line.size()};

            for (; i != e; ++i)
                if (not std::isspace(line[i]))
                    break;

            if (i != e and line[i] == '\\')
                continue;

            while (i != e) {
                m_position_stack.emplace_back(m_line, i);
                stack.emplace_back();

                for (; i != e; ++i) {
                    if (std::isspace(line[i]))
                        break;
                    else
                        stack.back() += line[i];
                }

                for (; i != e; ++i)
                    if (not std::isspace(line[i]))
                        break;
            }
        } while(m_is.good() and stack.empty());
    }
};

inline
bool set_constraint_name(problem& p, index id, operator_type op,
    const std::string& name)
{
    auto it = std::find_if(p.names_constraints.cbegin(),
                           p.names_constraints.cend(),
                           [&name](const auto& t)
                           {
                               return std::get<0>(t) == name;
                           });

    if (it != p.names_constraints.cend())
        return false;

    p.names_constraints.emplace_back(name, id, op);
    return true;
}

inline
std::string get_constraint_name(const problem& p, index id, operator_type op)
{
    auto it = std::find_if(p.names_constraints.cbegin(),
                           p.names_constraints.cend(),
                           [id, op](const auto& t)
                           {
                               return std::get<1>(t) == id
                                      and std::get<2>(t) == op;
                           });

    if (it != p.names_constraints.cend())
        return std::get<0>(*it);
    else
        return std::string{};
}

inline
index get_variable(problem& p, const std::string& name) noexcept
{
    auto it = std::find(p.vars.names.begin(), p.vars.names.end(), name);
    if (it != p.vars.names.end())
        return std::distance(p.vars.names.begin(), it);

    index id = std::distance(p.vars.names.cbegin(), p.vars.names.cend());
    p.vars.names.emplace_back(name);
    p.vars.values.emplace_back();

    return id;
}

inline
index get_variable_only(const problem& p, const std::string& name) noexcept
{
    auto it = std::find(p.vars.names.cbegin(), p.vars.names.cend(), name);
    if (it != p.vars.names.cend())
        return std::distance(p.vars.names.cbegin(), it);

    return -1;
}

std::string read_name(parser_stack& stack)
{
    std::string str = stack.top();
    std::string::size_type i = {0}, e = {str.length()};
    std::string ret;

    if (std::isalpha(str[i])) {
        ret += str[i];
        ++i;

        for (; i != e; ++i) {
            if (is_valid_character(str[i]))
                ret += str[i];
            else
                break;
        }

        stack.substr_front(i);

        return ret;
    }

    throw file_format_error(file_format_error::tag::bad_name,
                            stack.line(),
                            stack.column());
}

operator_type read_operator(parser_stack& stack)
{
    std::string str = stack.top();

    if (str[0] == '<') {
        if (str.size() > 1 and str[1] == '=') {
            stack.substr_front(2);
            return operator_type::less_equal;
        } else {
            stack.substr_front(1);
            return operator_type::less;
        }
    }

    if (str[0] == '>') {
        if (str.size() > 1 and str[1] == '=') {
            stack.substr_front(2);
            return operator_type::greater_equal;
        } else {
            stack.substr_front(1);
            return operator_type::greater;
        }
    }

    if (str[0] == '=') {
        stack.substr_front(1);
        return operator_type::equal;
    }

    throw file_format_error(file_format_error::tag::bad_operator,
                            stack.line(),
                            stack.column());
}

int read_integer(parser_stack& stack)
{
    std::string str = stack.top();

    int ret = {0};

    if (std::isdigit(str[0])) {
        ret = str[0] - '0';

        std::string::size_type i{1}, e{str.length()};
        for (; i != e; ++i) {
            if (std::isdigit(str[i])) {
                ret *= 10;
                ret += str[i] - '0';
            } else {
                break;
            }
        }

        stack.substr_front(i);
        return ret;
    }

    throw file_format_error(file_format_error::tag::bad_integer,
                            stack.line(),
                            stack.column());
}

std::tuple<std::string, int>
read_function_element(parser_stack& stack)
{
    bool negative {false};
    std::tuple<std::string, int> ret {std::string(), {1}};

    std::string str = stack.pop();

    {
        if (str[0] == '-' or str[0] == '+') {
            negative = (str[0] == '-');

            if (str.length() != 1)
                stack.push_front(str.substr(1, std::string::npos));
        } else {
            stack.push_front(str);
        }
    }

    str = stack.top();

    {
        if (std::isdigit(str[0])) {
            std::get<1>(ret) = read_integer(stack);
            if (negative)
                std::get<1>(ret) *= -1;
        } else if (negative) {
            std::get<1>(ret) = -1;
        }
    }

    str = stack.top();

    if (std::isalpha(str[0])) {
        std::get<0>(ret) = read_name(stack);
        return ret;
    }

    throw file_format_error(file_format_error::tag::bad_function_element,
                            stack.line(), stack.column());
}

objective_function_type
read_objective_function_type(parser_stack& stack)
{
    auto str = stack.top();

    std::string ret;
    std::string::size_type i {0}, e {str.length()};

    if (std::isalpha(str[i])) {
        for (; i != e; ++i) {
            if (std::isalpha(str[i]))
                ret += str[i];
            else
                break;
        }

        stack.substr_front(i);
    }

    if (iequals(ret, "maximize"))
        return objective_function_type::maximize;

    if (iequals(ret, "minimize"))
        return objective_function_type::minimize;

    throw file_format_error(file_format_error::tag::bad_objective_function_type,
                            stack.line(), stack.column());
}

objective_function
read_objective_function(parser_stack& stack, problem& p)
{
    objective_function ret;

    while (not stack.is_topic()) {
        auto elem = read_function_element(stack);

        if (std::get<0>(elem).empty())      // we read a constant
            ret.constant += std::get<1>(elem);
        else
            ret.elements.emplace_back(std::get<1>(elem),
                                      get_variable(p, std::get<0>(elem)));
    }

    return ret;
}

std::tuple<constraint, operator_type, std::string>
read_constraint(parser_stack& stack, problem& p)
{
    constraint cst;
    std::string label;

    if (std::isalpha(stack.peek())) {
        auto tmp = read_name(stack);

        if (stack.peek() == ':') {
            label = tmp;
            stack.substr_front(1);
        } else {
            cst.elements.emplace_back(1, get_variable(p, tmp));
        }
    }

    auto str = stack.top();

    while (not iequals(str, "binary") and not iequals(str, "binaries") and not iequals(str, "general")
           and not iequals(str, "end")) {
        if (is_operator(stack.peek())) {
            operator_type type = read_operator(stack);
            cst.value = read_integer(stack);
            return std::make_tuple(cst, type, label);
        } else {
            auto elem = read_function_element(stack);
            cst.elements.emplace_back(std::get<1>(elem),
                                      get_variable(p, std::get<0>(elem)));
        }

        str = stack.top();
    }

    throw file_format_error(file_format_error::tag::bad_constraint,
                            stack.line(), stack.column());
}

void read_constraints(parser_stack& stack, problem& p)
{
    auto str = stack.top();

    while (not iequals(str, "binary")
           and not iequals(str, "binaries")
           and not iequals(str, "bounds")
           and not iequals(str, "general") and not iequals(str, "end")) {
        auto cst = read_constraint(stack, p);

        index i = -1;
        switch (std::get<1>(cst)) {
        case operator_type::equal:
            p.equal_constraints.emplace_back(std::get<0>(cst));
            i = std::distance(p.equal_constraints.begin(),
                              p.equal_constraints.end());
            break;
        case operator_type::greater:
            p.greater_constraints.emplace_back(std::get<0>(cst));
            i = std::distance(p.greater_constraints.begin(),
                              p.greater_constraints.end());
            break;
        case operator_type::greater_equal:
            p.greater_equal_constraints.emplace_back(std::get<0>(
                                                                 cst));
            i = std::distance(p.greater_equal_constraints.begin(),
                              p.greater_equal_constraints.end());
            break;
        case operator_type::less:
            p.less_constraints.emplace_back(std::get<0>(cst));
            i = std::distance(p.less_constraints.begin(),
                              p.less_constraints.end());
            break;
        case operator_type::less_equal:
            p.less_equal_constraints.emplace_back(std::get<0>(cst));
            i = std::distance(p.less_equal_constraints.begin(),
                              p.less_equal_constraints.end());
            break;
        default:
            throw file_format_error(file_format_error::tag::unknown,
                                    stack.line(), stack.column());
        }

        i--;
        if (not std::get<2>(cst).empty() and i >= 0)
            set_constraint_name(p, i,
                                std::get<1>(cst),
                                std::get<2>(cst));

        str = stack.top();
    }
}

void apply_bound(int value, operator_type type, variable_value& variable)
{
    switch (type) {
    case operator_type::greater:
        variable.max = value;
        variable.max_equal = false;
        break;
    case operator_type::greater_equal:
        variable.max = value;
        variable.max_equal = true;
        break;
    case operator_type::less:
        variable.min = value;
        variable.min_equal = false;
        break;
    case operator_type::less_equal:
        variable.min = value;
        variable.min_equal = true;
        break;
    case operator_type::equal:
        variable.min = value;
        variable.max_equal = true;
        variable.max = value;
        variable.max_equal = true;
        break;
    case operator_type::undefined:
        break;
    }
}

void apply_bound(variable_value& variable, operator_type type, int value)
{
    switch (type) {
    case operator_type::greater:
        variable.min = value;
        variable.min_equal = false;
        break;
    case operator_type::greater_equal:
        variable.min = value;
        variable.min_equal = true;
        break;
    case operator_type::less:
        variable.max = value;
        variable.max_equal = false;
        break;
    case operator_type::less_equal:
        variable.max = value;
        variable.max_equal = true;
        break;
    case operator_type::equal:
        variable.min = value;
        variable.max_equal = true;
        variable.max = value;
        variable.max_equal = true;
        break;
    case operator_type::undefined:
        break;
    }
}

void read_bound(parser_stack& stack, problem& p)
{
    /*
     * If first character is a digit, tries to read the bound:
     * value [<|<=|=|>|>=] variable_name [<|<=|=|>|>=] value or
     * value [<|<=|=|>|>=] variable_name
     */
    if (std::isdigit(stack.peek())) {
        auto value_first = read_integer(stack);
        auto operator_type_first = read_operator(stack);
        auto variable = read_name(stack);
        auto id = get_variable(p, variable);

        apply_bound(value_first, operator_type_first, p.vars.values[id]);

        /*
         * If next character is a <, > or =, then tries to read second part of
         * the bound of: value [<|<=|=|>|>=] variable_name [<|<=|=|>|>=] value
         */
        if (is_operator(stack.peek())) {
            auto operator_type_second = read_operator(stack);
            auto value_second = read_integer(stack);

            apply_bound(p.vars.values[id], operator_type_second, value_second);
        }
    } else {
        /*
         * Tries to read the bound: variable_name [>|>=|=|<|<=] value
         */
        auto variable = read_name(stack);
        auto operator_type = read_operator(stack);
        auto value = read_integer(stack);
        auto id = get_variable(p, variable);

        apply_bound(p.vars.values[id], operator_type, value);
    }
}

void read_bounds(parser_stack& stack, problem& p)
{
    auto str = stack.top();

    while (not iequals(str, "binary")
           and not iequals(str, "binaries")
           and not iequals(str, "general")
           and not iequals(str, "end")) {
        read_bound(stack, p);
        str = stack.top();
    }
}

void read_binary(parser_stack& stack, problem& p)
{
    auto str = stack.top();

    while (not iequals(str, "general") and not iequals(str, "end")) {
        auto name = read_name(stack);
        auto id = get_variable_only(p, name);

        if (id < 0 or p.vars.values[id].type != variable_type::real)
            throw file_format_error(name, file_format_error::tag::unknown,
                                    stack.line(), stack.column());

        p.vars.values[id] = { 0, 1, variable_type::binary, true, true };

        str = stack.top();
    }
}

void read_general(parser_stack& stack, problem& p)
{
    auto str = stack.top();

    while (not iequals(str, "end")) {
        auto name = read_name(stack);
        auto id = get_variable_only(p, name);

        if (id < 0 or p.vars.values[id].type != variable_type::real)
            throw file_format_error(name, file_format_error::tag::unknown,
                                    stack.line(), stack.column());

        p.vars.values[id].type = variable_type::general;

        str = stack.top();
    }
}

problem read_problem(std::istream& is)
{
    problem p;
    parser_stack stack(is);
    std::string toek;

    p.type = read_objective_function_type(stack);
    p.objective = read_objective_function(stack, p);

    if (stack.is_subject_to())
        read_constraints(stack, p);

    if (stack.is_bounds())
        read_bounds(stack, p);

    if (stack.is_binary())
        read_binary(stack, p);

    if (stack.is_general())
        read_general(stack, p);

    if (stack.is_end()) {
        if (stack.empty())
            return p;
    }

    throw file_format_error("end",
                            file_format_error::tag::incomplete,
                            stack.line(), stack.column());
}

struct problem_writer
{
    const problem& p;
    std::ostream& os;
    int error;

    problem_writer(const problem& p_, std::ostream& os_)
        : p(p_)
        , os(os_)
        , error(0)
    {
        if (p.vars.names.empty()) {
            error = 1;
            return;
        }

        if (p.type == objective_function_type::maximize)
            os << "maximize\n";
        else
            os << "minimize\n";

        write_function_element(p.objective.elements);
        if (p.objective.constant < 0)
            os << p.objective.constant;
        else if (p.objective.constant > 0)
            os << '+' << p.objective.constant;

        os << '\n';

        os << "subject to\n";
        write_constraints();

        os << "bounds\n";
        write_bounds();

        bool have_binary = false;
        bool have_general = false;

        for (std::size_t i{0}, e{p.vars.names.size()}; i != e; ++i) {
            if (p.vars.values[i].type == variable_type::binary) {
                have_binary = true;
                if (have_general == true)
                    break;
            } else if (p.vars.values[i].type == variable_type::general) {
                have_general = true;
                if (have_binary == true)
                    break;
            }
        }

        if (have_binary) {
            os << "binary\n";
            for (std::size_t i{0}, e{p.vars.names.size()}; i != e; ++i)
                if (p.vars.values[i].type == variable_type::binary)
                    os << ' ' << p.vars.names[i] << '\n';
        }

        if (have_general) {
            os << "general\n";
            for (std::size_t i{0}, e{p.vars.names.size()}; i != e; ++i)
                if (p.vars.values[i].type == variable_type::general)
                    os << ' ' << p.vars.names[i] << '\n';
        }

        os << "end\n";
    }

    operator bool() const
    {
        return error == 0;
    }

private:
    void write_bounds() const
    {
        for (std::size_t i{0}, e{p.vars.names.size()}; i != e; ++i) {
            if (p.vars.values[i].min != 0)
                os << p.vars.names[i] << " >= " << p.vars.values[i].min << '\n';

            if (p.vars.values[i].max != std::numeric_limits<int>::max())
                os << p.vars.names[i] << " <= " << p.vars.values[i].max << '\n';
        }
    }

    void write_function_element(const std::vector<function_element>& f) const
    {
        for (auto& elem : f) {
            os << ((elem.factor < 0) ? "- " : "+ ");
            if (elem.factor != 1)
                os << std::abs(elem.factor) << ' ';

            os << p.vars.names[elem.variable_index] << ' ';
        }
    }

    void write_constraints() const
    {
        for (long i = 0, e = p.equal_constraints.size(); i != e; ++i) {
            auto str = get_constraint_name(p, i, operator_type::equal);
            if (not str.empty())
                os << str << ": ";
            write_function_element(p.equal_constraints[i].elements);
            os << " = " << p.equal_constraints[i].value << '\n';
        }

        for (long i = 0, e = p.greater_constraints.size(); i != e; ++i) {
            auto str = get_constraint_name(p, i, operator_type::greater);
            if (not str.empty())
                os << str << ": ";
            write_function_element(p.greater_constraints[i].elements);
            os << " > " << p.greater_constraints[i].value << '\n';
        }

        for (long i = 0, e = p.greater_equal_constraints.size(); i != e; ++i) {
            auto str = get_constraint_name(p, i, operator_type::greater_equal);
            if (not str.empty())
                os << str << ": ";
            write_function_element(p.greater_equal_constraints[i].elements);
            os << " >= " << p.greater_equal_constraints[i].value << '\n';
        }

        for (long i = 0, e = p.less_constraints.size(); i != e; ++i) {
            auto str = get_constraint_name(p, i, operator_type::less);
            if (not str.empty())
                os << str << ": ";
            write_function_element(p.less_constraints[i].elements);
            os << " < " << p.less_constraints[i].value << '\n';
        }

        for (long i = 0, e = p.less_equal_constraints.size(); i != e; ++i) {
            auto str = get_constraint_name(p, i, operator_type::less_equal);
            if (not str.empty())
                os << str << ": ";
            write_function_element(p.less_equal_constraints[i].elements);
            os << " <= " << p.less_equal_constraints[i].value << '\n';
        }
    }
};

} // namespace details

} // namespace lp

#endif
