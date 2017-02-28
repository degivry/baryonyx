/* Copyright (C) 2017 INRA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
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

#ifndef ORG_VLEPROJECT_LP_INEQUALITIES_1COEFF_HPP
#define ORG_VLEPROJECT_LP_INEQUALITIES_1COEFF_HPP

#include "lpcore-compare"
#include "lpcore-out"
#include "mitm.hpp"
#include "utils.hpp"
#include <Eigen/Core>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <unordered_map>

namespace lp {
namespace inequalities_1coeff {

using A_type = Eigen::MatrixXi;
using b_type = Eigen::Matrix<double, 2, Eigen::Dynamic>;
using c_type = Eigen::VectorXf;
using x_type = Eigen::VectorXi;
using P_type = Eigen::MatrixXf;
using pi_type = Eigen::VectorXf;
using u_type = Eigen::VectorXi;

enum class constraint_order
{
    none,
    reversing,
    random_sorting,
    infeasibility_decr,
    infeasibility_incr
};

const char*
constraint_order_to_string(constraint_order type)
{
    static const char* ret[] = { "none",
                                 "reversing",
                                 "random-sorting",
                                 "infeasibility-decr",
                                 "infeasibility-incr" };

    switch (type) {
        case inequalities_1coeff::constraint_order::none:
            return ret[0];
        case inequalities_1coeff::constraint_order::reversing:
            return ret[1];
        case inequalities_1coeff::constraint_order::random_sorting:
            return ret[2];
        case inequalities_1coeff::constraint_order::infeasibility_decr:
            return ret[3];
        case inequalities_1coeff::constraint_order::infeasibility_incr:
            return ret[4];
    }

    return nullptr;
}

double
get_real(const std::map<std::string, parameter>& params,
         std::string param,
         double def)
{
    auto it = params.find(param);
    if (it == params.cend())
        return def;

    if (it->second.type == parameter::tag::real)
        return it->second.d;

    if (it->second.type == parameter::tag::integer)
        return static_cast<double>(it->second.l);

    std::fprintf(
      stderr, " [FAIL] fail to convert parameter %s\n", param.c_str());

    return def;
}

long int
get_integer(const std::map<std::string, parameter>& params,
            std::string param,
            double def)
{
    auto it = params.find(param);
    if (it == params.cend())
        return def;

    if (it->second.type == parameter::tag::integer)
        return it->second.l;

    if (it->second.type == parameter::tag::real)
        return static_cast<long>(it->second.d);

    std::fprintf(
      stderr, " [FAIL] fail to convert parameter %s\n", param.c_str());

    return def;
}

inequalities_1coeff::constraint_order
get_constraint_order(const std::map<std::string, parameter>& params,
                     std::string param,
                     inequalities_1coeff::constraint_order def)
{
    auto it = params.find(param);
    if (it == params.cend())
        return def;

    if (it->second.type != parameter::tag::string)
        return def;

    if (it->second.s == "none")
        return inequalities_1coeff::constraint_order::none;
    if (it->second.s == "reversing")
        return inequalities_1coeff::constraint_order::reversing;
    if (it->second.s == "random-sorting")
        return inequalities_1coeff::constraint_order::random_sorting;
    if (it->second.s == "infeasibility-decr")
        return inequalities_1coeff::constraint_order::infeasibility_decr;
    if (it->second.s == "infeasibility-incr")
        return inequalities_1coeff::constraint_order::infeasibility_incr;

    return def;
}

struct parameters
{
    parameters(const std::map<std::string, parameter>& params)
      : order(get_constraint_order(params,
                                   "constraint-order",
                                   constraint_order::none))
      , theta(get_real(params, "theta", 0.5))
      , delta(get_real(params, "delta", 0.5))
      , limit(get_integer(params, "limit", 100l))
      , kappa_min(get_real(params, "kappa-min", 0.0))
      , kappa_step(get_real(params, "kappa-step", 0.0001))
      , kappa_max(get_real(params, "kappa-max", 0.6))
      , alpha(get_real(params, "alpha", 2.0))
      , w(get_integer(params, "w", 20l))
      , serialize(get_integer(params, "serialize", 0l))
    {
    }

    void print() const noexcept
    {
        printf("* solver inequalities_1coeff_wedelin\n"
               "  - constraint-order: %s\n"
               "  - theta: %f\n"
               "  - delta: %f\n"
               "  - limit: %ld\n"
               "  - kappa-min: %f\n"
               "  - kappa-step: %f\n"
               "  - kappa-max: %f\n"
               "  - alpha: %f\n"
               "  - w: %ld\n"
               "  - serialise: %d\n",
               constraint_order_to_string(order),
               theta,
               delta,
               limit,
               kappa_min,
               kappa_step,
               kappa_max,
               alpha,
               w,
               serialize);
    }

    constraint_order order;
    double theta;
    double delta;
    long int limit;
    double kappa_min;
    double kappa_step;
    double kappa_max;
    double alpha;
    long int w;
    bool serialize;
};

struct maximize_tag
{
};

struct minimize_tag
{
};

template <typename iteratorT>
void
calculator_sort(iteratorT begin, iteratorT end, minimize_tag)
{
    std::sort(begin, end, [](const auto& lhs, const auto& rhs) {
        return lhs.value < rhs.value;
    });
}

template <typename iteratorT>
void
calculator_sort(iteratorT begin, iteratorT end, maximize_tag)
{
    std::sort(begin, end, [](const auto& lhs, const auto& rhs) {
        return rhs.value < lhs.value;
    });
}

bool
stop_iterating(double value, minimize_tag)
{
    return value >= 0;
}

bool
stop_iterating(double value, maximize_tag)
{
    return value <= 0;
}

template <typename modeT>
struct constraint_calculator
{
    using mode_type = modeT;

    struct r_data
    {
        r_data(double value_, index index_)
          : value(value_)
          , id(index_)
        {
        }

        double value;
        index id;
    };

    A_type& A;
    b_type& b;
    c_type& cost;
    x_type& x;
    P_type& P;
    pi_type& pi;
    std::vector<index> I; // Stores variables with non null coefficient.
    std::vector<r_data> r;
    std::vector<index> C; // Stores variables with negative coefficient.
    index m;
    index n;

    constraint_calculator(index k,
                          index m_,
                          index n_,
                          A_type& A_,
                          b_type& b_,
                          c_type& c_,
                          x_type& x_,
                          P_type& P_,
                          pi_type& pi_)
      : A(A_)
      , b(b_)
      , cost(c_)
      , x(x_)
      , P(P_)
      , pi(pi_)
      , m(m_)
      , n(n_)
    {
        for (index i = 0; i != n; ++i) {
            if (A(k, i)) {
                I.emplace_back(i);
                r.emplace_back(0.0, i);
            }

            if (A(k, i) < 0)
                C.emplace_back(i);
        }
    }

    void serialize(index k, std::ostream& os) const
    {
        os << "[P(" << k << ", i): ";
        for (auto i : I)
            os << P(k, i) << ' ';
        os << "] ";

        os << b(0, k) << " <= ";

        for (index i = 0, endi = numeric_cast<index>(I.size()); i != endi; ++i)
            os << A(k, I[i]) << ' ';
        os << " <= " << b(1, k) << '\n';
    }

    void update_row(index k, double kappa, double delta, double theta)
    {
        for (auto i : I)
            P(k, i) *= theta;

        for (std::size_t i{ 0 }, endi{ I.size() }; i != endi; ++i) {
            double sum_a_pi{ 0 };
            double sum_a_p{ 0 };

            for (index h = 0; h != m; ++h) {
                if (A(h, I[i])) {
                    sum_a_pi += A(h, I[i]) * pi(h);
                    sum_a_p += A(h, I[i]) * P(h, I[i]);
                }
            }

            r[i].value = cost(I[i]) - sum_a_pi - sum_a_p;
            r[i].id = I[i];

            // std::cerr << "update_row: " << r[i].id << "=" << I[i] << '\n';
        }

        /*
         * Negate reduced costs and coefficients of these variables.
         */

        for (std::size_t i{ 0 }, endc{ C.size() }; i != endc; ++i) {
            int variable = C[i];

            auto it = std::find_if(r.begin(), r.end(), [variable](auto elem) {
                return elem.id == variable;
            });

            assert(it != r.end());

            it->value *= -1;
            A(k, variable) *= -1;
            P(k, variable) *= -1;
        }

        calculator_sort(r.begin(), r.end(), mode_type());

        b(0, k) += C.size();
        b(1, k) += C.size();

        assert(b(0, k) >= 0);
        assert(b(0, k) <= b(1, k));

        /*
         * The bkmin and bkmax constraint bounds are not equal and can be
         * assigned to -infinity or +infinity. We have to scan the r vector
         * and search a value j such as b(0, k) <= Sum A(k, r[j]) < b(1,
         * k).
         */
        index i{ 0 }, selected{ -1 }, first, second;
        const index endi{ numeric_cast<index>(r.size()) };
        int sum{ 0 };

        // std::cerr << "constraint " << k << " endi: " << endi << '\n';
        // std::cerr << "   " << b(0, k) << " " << b(1, k) << '\n';

        for (; i != endi; ++i) {
            sum += A(k, r[i].id);
            // std::cerr << "   " << sum << '\n';
            assert(A(k, r[i].id) == 1);
            if (b(0, k) <= sum)
                break;
        }

        //
        // The sum seems good. The constraint is valid we continue to scan. If
        // the minimum constraints is invalid, we assign 0 to all variable by
        // settings the selected value to -1.
        //
        if (b(0, k) <= sum and sum <= b(1, k)) {
            // std::cerr << "   begin sum:" << sum << " b(0,k):" << b(0, k)
            //           << " selected:" << i << '\n';

            if (b(0, k) > sum)
                throw solver_error(solver_error::tag::unrealisable_constraint);

            selected = i;
            for (; i != endi; ++i) {
                sum += A(k, r[i].id);

                // std::cerr << "   " << sum << '\n';

                if (sum <= b(1, k)) {
                    if (stop_iterating(r[i].value, mode_type()))
                        break;
                    ++selected;
                } else
                    break;
            }

            if (i == endi)
                throw solver_error(solver_error::tag::unrealisable_constraint);

            assert(selected < endi);

            // std::cerr << "   finish sum:" << sum << " b(1,k):" << b(1, k)
            // << " selected:" << selected << '\n';

            first = selected;
            second = selected + 1;

        } else {
            first = 0;
            second = 1;
        }

        if (second >= endi) {
            first = selected - 1;
            second = selected;
        }

        // std::cerr << "   finally first=" << first << " second=" << second
        //           << '\n';

        pi(k) += ((r[first].value + r[second].value) / 2.0);

        double d = delta + ((kappa / (1.0 - kappa)) *
                            (r[second].value - r[first].value));

        index j{ 0 };
        // std::cerr << "   Assign 1 from " << j << " to " << selected << '\n';
        for (; j <= selected; ++j) {
            // std::cerr << "   Assign 1 to " << r[j].id << '\n';
            x(r[j].id) = 1;
            P(k, r[j].id) += d;
        }

        // std::cerr << "   Assign 0 from " << j << " to " << endi << '\n';
        for (; j != endi; ++j) {
            // std::cerr << "   Assign 0 to " << r[j].id << '\n';
            x(r[j].id) = 0;
            P(k, r[j].id) -= d;
        }

        b(0, k) -= C.size();
        b(1, k) -= C.size();

        /*
         * Clean up: correct negated costs and adjust value of negated
         * variables.
         */
        for (std::size_t i{ 0 }, e{ C.size() }; i != e; ++i) {
            A(k, C[i]) *= -1;
            P(k, C[i]) *= -1;

            // std::cerr << "   Reassign " << 1 - x[C[i]] << " to " << C[i]
            // << '\n';

            x[C[i]] = 1 - x[C[i]];
        }
    }
};

struct merged_constraint
{
    merged_constraint(const std::vector<lp::function_element>& elements_,
                      int min_,
                      int max_)
      : elements(elements_)
      , min(min_)
      , max(max_)
    {
    }

    std::vector<lp::function_element> elements;
    int min;
    int max;
};

// template <typename functionT>
// void
// show_mc(const functionT& f)
// {
//     for (auto& elem : f)
//         printf("%d * %ld ", elem.factor, elem.variable_index);
//     printf("\n");
// }

class my_hash
{
public:
    size_t operator()(const std::vector<lp::function_element>& fct) const
    {
        size_t sum{ 0 };

        sum =
          std::accumulate(fct.begin(),
                          fct.end(),
                          0,
                          [](std::size_t i, const lp::function_element& f) {
                              return i * 100 + f.variable_index;
                          });

        return sum;
    }
};

std::vector<merged_constraint>
make_merged_constraints(const lp::problem& pb)
{
    std::vector<merged_constraint> ret;
    std::unordered_map<std::vector<lp::function_element>, std::size_t, my_hash>
      cache;

    for (const auto& elem : pb.equal_constraints) {
        cache.emplace(elem.elements, ret.size());
        ret.emplace_back(elem.elements,
                         numeric_cast<int>(std::lround(elem.value)),
                         numeric_cast<int>(std::lround(elem.value)));
    }

    for (const auto& elem : pb.less_equal_constraints) {
        auto it = cache.find(elem.elements);
        if (it == cache.end()) {
            cache.emplace(elem.elements, ret.size());
            ret.emplace_back(elem.elements,
                             std::numeric_limits<int>::min(),
                             numeric_cast<int>(std::lround(elem.value)));
        } else {
            ret[it->second].max = std::min(
              ret[it->second].max, numeric_cast<int>(std::lround(elem.value)));
        }
    }

    for (const auto& elem : pb.greater_equal_constraints) {
        auto it = cache.find(elem.elements);
        if (it == cache.end()) {
            cache.emplace(elem.elements, ret.size());
            ret.emplace_back(
              elem.elements, elem.value, std::numeric_limits<int>::max());
        } else {
            ret[it->second].min = std::max(
              ret[it->second].min, numeric_cast<int>(std::lround(elem.value)));
        }
    }

    const std::size_t nb{ pb.equal_constraints.size() +
                          pb.less_equal_constraints.size() +
                          pb.greater_equal_constraints.size() - ret.size() };

    printf("  - removed constraints: %ld\n", numeric_cast<long int>(nb));

    printf("  - constraints stored in: constraints.tmp.lp\n");
    std::ofstream ofs("constraints.tmp.lp");
    for (auto& elem : ret) {
        ofs << elem.min << " <= ";

        for (auto& f : elem.elements)
            ofs << ((f.factor < 0) ? "- " : "+ ") << f.variable_index << ' ';

        ofs << " <= " << elem.max << '\n';
    }

    // std::vector<int> vars(pb.vars.values.size(), 0);

    // for (auto& elem : ret)
    //     for (auto& f : elem.elements)
    //         vars[f.variable_index]++;

    // std::sort(
    //   ret.begin(), ret.end(), [vars](const auto& lhs, const auto& rhs) {
    //       long sumlhs{ 1 };
    //       long sumrhs{ 1 };

    //       for (auto& f : lhs.elements)
    //           sumlhs *= vars[f.variable_index];

    //       for (auto& f : rhs.elements)
    //           sumrhs *= vars[f.variable_index];

    //       return sumrhs < sumlhs;
    //   });

    return ret;
}

template <typename modeT>
class solver
{
    std::vector<constraint_calculator<modeT>> row_updaters;
    std::vector<index> R;
    index m;
    index n;
    A_type A;
    b_type b;
    c_type c;
    x_type x;
    P_type P;
    pi_type pi;
    u_type u;
    const lp::problem& pb;
    bool solution_found;
    std::default_random_engine rng;

public:
    using mode_type = modeT;

    solver(const lp::problem& pb, const std::vector<merged_constraint>& csts)
      : m(csts.size())
      , n(pb.vars.values.size())
      , A(A_type::Zero(m, n))
      , b(b_type::Zero(2, m))
      , c(c_type::Zero(n))
      , x(x_type::Zero(n))
      , P(P_type::Zero(m, n))
      , pi(pi_type::Zero(m))
      , u(u_type::Zero(n))
      , pb(pb)
      , solution_found(false)
      , rng(std::chrono::system_clock::now().time_since_epoch().count())
    {
        for (std::size_t i{ 0 }, e = pb.vars.values.size(); i != e; ++i)
            u(i) = pb.vars.values[i].max;

        //
        // Compute bkmin and bkmax acccording to the value of the constraints
        // (infinity or constant) and the number of element in the expression
        // (avoid the bkmax += C.size() in inequalities).
        //

        for (std::size_t i{ 0 }, e{ csts.size() }; i != e; ++i) {
            index lower{ 0 }, upper{ 0 };
            // index lower{ -1 }, upper{ 1 };
            for (const auto& cst : csts[i].elements) {
                A(i, cst.variable_index) = cst.factor;
                if (cst.factor < 0)
                    lower--;
                if (cst.factor > 0)
                    upper++;
            }

            if (csts[i].min == std::numeric_limits<int>::min())
                b(0, i) = lower;
            else
                b(0, i) = csts[i].min;

            if (csts[i].max == std::numeric_limits<int>::max())
                b(1, i) = upper;
            else
                b(1, i) = csts[i].max;
        }

        for (const auto& elem : pb.objective.elements) {
            c(elem.variable_index) += elem.factor;
            x(elem.variable_index) = c(elem.variable_index) <= 0 ? 1 : 0;
        }

        for (index k = 0; k != m; ++k)
            row_updaters.emplace_back(k, m, n, A, b, c, x, P, pi);
    }

    void serialize(std::ostream& os) const
    {
        std::vector<index> vec;

        for (index k{ 0 }; k != m; ++k) {
            int v = 0;

            for (index i{ 0 }; i != n; ++i)
                v += A(k, i) * x(i);

            if (not(b(0, k) <= v and v <= b(1, k)))
                vec.push_back(k);
        }

        for (auto it{ vec.cbegin() }, et{ vec.cend() }; it != et; ++it)
            row_updaters[*it].serialize(*it, os);
    }

    std::size_t compute_none(double kappa, double delta, double theta)
    {
        R.clear();

        for (index k{ 0 }; k != m; ++k) {
            int v = 0;

            for (index i{ 0 }; i != n; ++i)
                v += A(k, i) * x(i);

            if (not(b(0, k) <= v and v <= b(1, k)))
                R.push_back(k);
        }

        for (auto it{ R.cbegin() }, et{ R.cend() }; it != et; ++it)
            row_updaters[*it].update_row(*it, kappa, delta, theta);

        if (R.empty())
            solution_found = true;

        return R.size();
    }

    std::size_t compute_reversing(double kappa, double delta, double theta)
    {
        R.clear();

        for (index k{ 0 }; k != m; ++k) {
            int v = 0;

            for (index i{ 0 }; i != n; ++i)
                v += A(k, i) * x(i);

            if (not(b(0, k) <= v and v <= b(1, k)))
                R.push_back(k);
        }

        for (auto it{ R.crbegin() }, et{ R.crend() }; it != et; ++it)
            row_updaters[*it].update_row(*it, kappa, delta, theta);

        if (R.empty())
            solution_found = true;

        return R.size();
    }

    std::size_t compute_random_sorting(double kappa,
                                       double delta,
                                       double theta)
    {
        R.clear();

        for (index k{ 0 }; k != m; ++k) {
            int v = 0;

            for (index i{ 0 }; i != n; ++i)
                v += A(k, i) * x(i);

            if (not(b(0, k) <= v and v <= b(1, k)))
                R.push_back(k);
        }

        std::shuffle(R.begin(), R.end(), rng);

        for (index k : R)
            row_updaters[k].update_row(k, kappa, delta, theta);

        if (R.empty())
            solution_found = true;

        return R.size();
    }

    std::size_t compute_infeasibility_decr(double kappa,
                                           double delta,
                                           double theta)
    {
        std::vector<std::pair<index, index>> currentR;

        for (index k{ 0 }; k != m; ++k) {
            int v = 0;

            for (index i{ 0 }; i != n; ++i)
                v += A(k, i) * x(i);

            if (not(b(0, k) <= v and v <= b(1, k))) {
                currentR.emplace_back(k, 0);

                if (v < b(0, k))
                    currentR.back().second = b(0, k) - v;
                else
                    currentR.back().second = v - b(1, k);
            }
        }

        std::sort(currentR.begin(),
                  currentR.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return rhs.second < lhs.second;
                  });

        std::bernoulli_distribution d(.5);
        std::size_t i{ 0 };
        const std::size_t e{ currentR.size() };

        for (; i != e; ++i) {
            if (i + 1 == e)
                break;

            if (currentR[i].second == currentR[i + 1].second)
                if (d(rng))
                    std::swap(currentR[i].first, currentR[i + 1].second);
        }

        for (i = 0; i != e; ++i)
            row_updaters[currentR[i].first].update_row(
              currentR[i].first, kappa, delta, theta);

        if (currentR.empty())
            solution_found = true;

        return currentR.size();
    }

    std::size_t compute_infeasibility_incr(double kappa,
                                           double delta,
                                           double theta)
    {
        std::vector<std::pair<index, index>> currentR;

        for (index k{ 0 }; k != m; ++k) {
            int v = 0;

            for (index i{ 0 }; i != n; ++i)
                v += A(k, i) * x(i);

            if (not(b(0, k) <= v and v <= b(1, k))) {
                currentR.emplace_back(k, 0);

                if (v < b(0, k))
                    currentR.back().second = b(0, k) - v;
                else
                    currentR.back().second = v - b(1, k);
            }
        }

        std::sort(currentR.begin(),
                  currentR.end(),
                  [](const auto& lhs, const auto& rhs) {
                      return lhs.second < rhs.second;
                  });

        std::bernoulli_distribution d(.5);
        std::size_t i{ 0 };
        const std::size_t e{ currentR.size() };

        for (; i != e; ++i) {
            if (i + 1 == e)
                break;

            if (currentR[i].second == currentR[i + 1].second)
                if (d(rng))
                    std::swap(currentR[i].first, currentR[i + 1].second);
        }

        for (i = 0; i != e; ++i)
            row_updaters[currentR[i].first].update_row(
              currentR[i].first, kappa, delta, theta);

        if (currentR.empty())
            solution_found = true;

        return currentR.size();
    }

    double compute_value() const
    {
        double ret = pb.objective.constant;

        for (auto& elem : pb.objective.elements)
            ret += elem.factor * x(elem.variable_index);

        return ret;
    }

    result results() const
    {
        result ret;
        ret.method = "inequalities_101coeff_wedelin";
        ret.variables = n;
        ret.constraints = m;
        ret.value = compute_value();
        ret.solution_found = solution_found;
        ret.variable_name.resize(n);
        ret.variable_value.resize(n, 0);

        for (index i = 0; i != n; ++i) {
            ret.variable_name[i] = pb.vars.names[i];
            ret.variable_value[i] = x(i);
        }

        return ret;
    }
};

template <typename modeT>
inline result
run(const problem& pb, const parameters& p)
{
    using mode_type = modeT;

    auto begin = std::chrono::steady_clock::now();
    long int i2{ 0 };
    double kappa_old{ 0 };
    double kappa = p.kappa_min;

    solver<mode_type> slv(pb, make_merged_constraints(pb));
    result best;
    best.remaining_constraints = std::numeric_limits<index>::max();

    for (long int i{ 0 }; i < p.limit; ++i) {
        index remaining;
        switch (p.order) {
            case constraint_order::none:
                remaining = slv.compute_none(kappa, p.delta, p.theta);
                break;
            case constraint_order::reversing:
                remaining = slv.compute_reversing(kappa, p.delta, p.theta);
                break;
            case constraint_order::random_sorting:
                remaining =
                  slv.compute_random_sorting(kappa, p.delta, p.theta);
                break;
            case constraint_order::infeasibility_decr:
                remaining =
                  slv.compute_infeasibility_decr(kappa, p.delta, p.theta);
                break;
            case constraint_order::infeasibility_incr:
                remaining =
                  slv.compute_infeasibility_incr(kappa, p.delta, p.theta);
                break;
        }

        auto current = slv.results();
        current.loop = i;
        current.remaining_constraints = remaining;
        current.begin = begin;
        current.end = std::chrono::steady_clock::now();

        if (remaining < best.remaining_constraints) {
            auto t = std::chrono::duration_cast<std::chrono::duration<double>>(
              current.end - current.begin);

            // printf("\r  - constraints remaining: %ld/%ld at %fs",
            //        remaining,
            //        current.constraints,
            //        t.count());

            // fflush(stdout);

            printf("  - constraints remaining: %ld/%ld at %fs\n",
                   remaining,
                   current.constraints,
                   t.count());

            best = current;

            if (p.serialize) {
                std::ofstream ofs("current-solver.lp.dat");
                slv.serialize(ofs);
            }
        }

        if (current.solution_found) {
            std::cout << '\n' << current << '\n';
            return current;
        }

        if (i2 <= p.w) {
            kappa = p.kappa_min;
            i2++;
        } else {
            i2 = 0;
            kappa = kappa_old +
                    p.kappa_step *
                      std::pow(static_cast<double>(remaining) /
                                 static_cast<double>(current.constraints),
                               p.alpha);
            kappa_old = kappa;
        }

        if (kappa > p.kappa_max) {
            std::cout << "\nFail: kappa-max reached\n";
            return best;
        }
    }

    std::cout << "\nFail: limit reached\n";
    return best;
}

} // inequalities_1coeff

inline result
inequalities_1coeff_wedelin(const problem& pb,
                            const std::map<std::string, parameter>& params)
{
    namespace ine_1 = lp::inequalities_1coeff;
    ine_1::parameters p(params);
    p.print();

    if (pb.type == lp::objective_function_type::maximize)
        return ine_1::run<ine_1::maximize_tag>(pb, p);

    return ine_1::run<ine_1::minimize_tag>(pb, p);
}

} // lp

#endif
