/* Copyright (C) 2016 INRA
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

#ifndef ORG_VLEPROJECT_LP_GENERALIZED_WEDELIN_HPP
#define ORG_VLEPROJECT_LP_GENERALIZED_WEDELIN_HPP

#include "mitm.hpp"
#include "utils.hpp"
#include <Eigen/Core>
#include <iterator>

namespace lp {
namespace generalized {

using A_type = Eigen::MatrixXi;
using b_type = Eigen::Matrix<int, 2, Eigen::Dynamic>;
using c_type = Eigen::VectorXf;
using x_type = Eigen::VectorXi;
using P_type = Eigen::MatrixXf;
using pi_type = Eigen::VectorXf;

struct maximize_tag {
};
struct minimize_tag {
};

template <typename iteratorT>
void
simple_constraint_calculator_sort(iteratorT begin, iteratorT end, minimize_tag)
{
    std::sort(begin, end, [](const auto& lhs, const auto& rhs) {
        return lhs.value < rhs.value;
    });
}

template <typename iteratorT>
void
simple_constraint_calculator_sort(iteratorT begin, iteratorT end, maximize_tag)
{
    std::sort(begin, end, [](const auto& lhs, const auto& rhs) {
        return rhs.value < lhs.value;
    });
}

template <typename modeT>
struct constraint_calculator {
    using mode_type = modeT;

    virtual void update_row(index k,
                            double kappa,
                            double delta,
                            double theta) = 0;

    virtual ~constraint_calculator() {}
};

template <typename modeT>
struct simple_constraint_calculator : public constraint_calculator<modeT> {
    using mode_type = typename constraint_calculator<modeT>::mode_type;

    struct r_data {
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
    c_type& c;
    x_type& x;
    P_type& P;
    pi_type& pi;
    std::vector<index> I;
    std::vector<std::vector<r_data>> r;
    index m;
    index n;

    simple_constraint_calculator(const problem& pb,
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
      , c(c_)
      , x(x_)
      , P(P_)
      , pi(pi_)
      , m(m_)
      , n(n_)
    {
        for (const auto& elem : pb.objective.elements)
            x(elem.variable_index) = c(elem.variable_index) <= 0;

        for (index i = 0; i != m; ++i) {
            for (const auto& elem : pb.equal_constraints[i].elements) {
                I.emplace_back(elem.variable_index);
                r.emplace_back(0.0, elem.variable_index);
            }
        }
    }

    virtual void update_row(index k,
                            double kappa,
                            double delta,
                            double theta) override
    {
        for (auto i : I)
            P(k, i) *= theta;

        for (index i = 0, endi = numeric_cast<index>(I.size()); i != endi;
             ++i) {
            double sum_a_pi{ 0 };
            double sum_a_p{ 0 };

            for (index h = 0; h != m; ++h) {
                if (A(h, I[i])) {
                    sum_a_pi += A(h, I[i]) * pi(h);
                    sum_a_p += A(h, I[i]) * P(h, I[i]);
                }
            }

            r[i].value = c(I[i]) - sum_a_pi - sum_a_p;
            r[i].id = I[i];
        }

        simple_constraint_calculator_sort(r.begin(), r.end(), mode_type());

        pi(k) += ((r[b(k)].value + r[b(k) - 1].value) / 2.0);

        double d = delta + ((kappa / (1.0 - kappa)) *
                            (r[b(k) - 1].value - r[b(k)].value));

        for (index j = 0; j < b(k); ++j) {
            x(r[j].id) = 1;
            P(k, r[j].id) -= d;
        }

        for (index j = b(k), endj = numeric_cast<index>(I.size()); j != endj;
             ++j) {
            x(r[j].id) = 0;
            P(k, r[j].id) -= -d;
        }
    }
};

// struct advanded_constraint_calculator : public constraint_calculator {
// public:
// };

// struct generalized_constraint_calculator : public constraint_calculator {
// public:
// };

template <typename modeT>
class solver {
    std::vector<constraint_calculator<modeT>*> row_updaters;
    index m;
    index n;
    A_type A;
    b_type b;
    c_type c;
    x_type x;
    P_type P;
    pi_type pi;

public:
    solver(const lp::problem& pb)
      : m(pb.equal_constraints.size() + pb.greater_equal_constraints.size() +
          pb.greater_constraints.size() +
          pb.less_equal_constraints.size() +
          pb.less_constraints.size())
      , n(pb.vars.values.size())
      , A(A_type::Zero(m, n))
      , b(b_type::Zero(2, m))
      , c(c_type::Zero(n))

      , x(x_type::Zero(n))
      , P(P_type::Zero(m, n))
      , pi(pi_type::Zero(m, n))
    {
        for (lp::index i = 0; i != m; ++i) {
            for (const auto& elem : pb.equal_constraints[i].elements) {
                auto j = elem.variable_index;
                A(i, j) = 1;
            }
        }

        index i = 0;
        for (const auto& elem : pb.equal_constraints) {
            b(0, i) = elem.min;
            b(1, i) = elem.max;
            ++i;
        }

        for (const auto& elem : pb.greater_equal_constraints) {
            b(0, i) = elem.min;
            b(1, i) = elem.max;
            ++i;
        }

        for (const auto& elem : pb.greater_equal_constraints) {
            b(0, i) = elem.min;
            b(1, i) = elem.max;
            ++i;
        }

        for (const auto& elem : pb.less_equal_constraints) {
            b(0, i) = elem.min;
            b(1, i) = elem.max;
            ++i;
        }

        for (const auto& elem : pb.less_equal_constraints) {
            b(0, i) = elem.min;
            b(1, i) = elem.max;
            ++i;
        }

        for (const auto& elem : pb.objective.elements)
            c(elem.variable_index) = elem.factor;
    }

    virtual ~solver()
    {
        std::for_each(row_updaters.begin(),
                      row_updaters.end(),
                      [](auto* elem) { delete elem; });
    }

    bool compute(double kappa, double delta, double theta)
    {
        (void)kappa;
        (void)delta;
        (void)theta;
        return true;
    }

    result results() const
    {
        result ret;
        ret.optimal = false;

        return ret;
    }
};

} // generalized

inline result
generalized_wedelin(double kappa,
                    double delta,
                    double theta,
                    long limit,
                    const problem& pb)
{
    using maximize_solver = generalized::solver<generalized::maximize_tag>;
    using minimize_solver = generalized::solver<generalized::minimize_tag>;
    long int i{ 0 };

    if (pb.type == lp::objective_function_type::maximize) {
        maximize_solver slv(pb);
        while (not slv.compute(kappa, delta, theta and i++ != limit))
            ;

        return slv.results();
    }

    minimize_solver slv(pb);

    while (not slv.compute(kappa, delta, theta and i++ != limit))
        ;

    return slv.results();
}

} // lp

// namespace lp {
// namespace details {

// Eigen::MatrixXi
// make_inequality_a(index m, index n, const problem& pb)
// {
//     Eigen::MatrixXi A(Eigen::MatrixXi::Zero(m, n));

//     lp::index i {0};
//     for (const auto& cst : pb.equal_constraints) {
//         for (const auto& elem : cst.elements) {
//             auto j = elem.variable_index;
//             A(i, j) = elem.factor;
//         }
//         ++i;
//     }

//     for (const auto& cst : pb.greater_equal_constraints) {
//         for (const auto& elem : cst.elements) {
//             auto j = elem.variable_index;
//             A(i, j) = elem.factor;
//         }
//         ++i;
//     }

//     for (const auto& cst : pb.less_equal_constraints) {
//         for (const auto& elem : cst.elements) {
//             auto j = elem.variable_index;
//             A(i, j) = elem.factor;
//         }
//         ++i;
//     }

//     Ensures(i == m, "make_inequality i != n");

//     return A;
// }

// Eigen::Matrix<int, 2, Eigen::Dynamic>
// make_inequality_b(index m, const problem& p)
// {
//     using matrix = Eigen::Matrix<int, 2, Eigen::Dynamic>;

//     matrix b(matrix::Zero(2, m));

//     lp::index i = 0;
//     for (const auto& elem : p.equal_constraints) {
//         b(0, i) = elem.value;
//         b(1, i) = elem.value;
//         ++i;
//     }

//     for (const auto& elem : p.greater_equal_constraints) {
//         b(0, i) = elem.value;
//         b(1, i) = std::numeric_limits<int>::max();
//         ++i;
//     }

//     for (const auto& elem : p.greater_equal_constraints) {
//         b(0, i) = 0; // std::numeric_limits<int>::min();
//         b(1, i) = elem.value;
//         ++i;
//     }

//     Ensures(i == m, "make_inequality_b != n");

//     return b;
// }

// /**
//  * Remove lower bound < 0 without changing the problem's structure other
//  * than modifying the right-hand side and adding a constant to the
//  * objective function.
//  *
//  * \param pb Initial problem definition.
//  *
//  * \return New problem without lower bound variable less than 0.
//  */
// inline
// problem adapt_problem(const problem& pb)
// {
//     problem ret(pb);
//     int constant {0};

//     for (std::size_t i {0}, e{ret.vars.values.size()}; i != e; ++i) {
//         if (ret.vars.values[i].min < 0) {
//             constant += ret.vars.values[i].min;
//             ret.vars.values[i].max -= ret.vars.values[i].min;
//             ret.vars.values[i].min = 0;
//         }
//     }

//     ret.objective.constant = constant;

//     return ret;
// }

// Eigen::VectorXf
// make_generalized_c(index n, const problem& p)
// {
//     Eigen::VectorXf c = Eigen::VectorXf::Zero(n);

//     for (const auto& elem : p.objective.elements)
//         c(elem.variable_index) = elem.factor;

//     return c;
// }

// struct maximize_generalized_tag {};
// struct minimize_generalized_tag {};

// template<typename T>
// struct solver_generalized_tag {
//     using type = T;
// };

// template<typename iteratorT>
// void sort_generalized(iteratorT begin, iteratorT end,
//                       minimize_generalized_tag)
// {
//     std::sort(begin, end,
//               [](const auto& lhs, const auto& rhs)
//               {
//                   return lhs.value < rhs.value;
//               });
// }

// template<typename iteratorT>
// void sort_generalized(iteratorT begin, iteratorT end,
//                       maximize_generalized_tag)
// {
//     std::sort(begin, end,
//               [](const auto& lhs, const auto& rhs)
//               {
//                   return rhs.value < lhs.value;
//               });
// }

// template<typename modeT>
// class generalized_wedelin
// {
//     struct r_data {
//         r_data(double value_, index index_)
//             : value(value_)
//             , id(index_)
//         {}

//         double value;
//         index id;
//     };

//     using tag = modeT;
//     index m, n;
//     problem pb;                         // a copy to change the problem
//                                         // (remove lower bound < 0)
//     Eigen::MatrixXi A;
//     Eigen::Matrix<int, 2, Eigen::Dynamic> b;
//     Eigen::VectorXf c;
//     Eigen::VectorXi x;
//     Eigen::VectorXi u;
//     Eigen::MatrixXf P;
//     Eigen::VectorXf pi;
//     std::vector<std::vector<index>> I; // Variable with non-zero coefficient
//     std::vector<std::vector<index>> C; // Variable with negative coefficient
//     std::vector<std::vector<r_data>> r;
//     double kappa, delta, theta;
//     index loop;
//     bool optimal;

//     void make_c_i_r()
//     {
//         I.clear();
//         C.clear();
//         I.resize(m);
//         C.resize(m);
//         r.resize(m);

//         lp::index i {0};
//         for (const auto& cst : pb.equal_constraints) {
//             for (const auto& elem : cst.elements) {
//                 auto j = elem.variable_index;
//                 if (elem.factor < 0)
//                     C[i].push_back(j);
//                 if (elem.factor)
//                     I[i].push_back(j);
//                 r[i].emplace_back(0.0, j);
//             }

//             ++i;
//         }

//         for (const auto& cst : pb.greater_equal_constraints) {
//             for (const auto& elem : cst.elements) {
//                 auto j = elem.variable_index;
//                 if (elem.factor < 0)
//                     C[i].push_back(j);
//                 if (elem.factor)
//                     I[i].push_back(j);
//                 r[i].emplace_back(0.0, j);
//             }

//             ++i;
//         }

//         for (const auto& cst : pb.less_equal_constraints) {
//             for (const auto& elem : cst.elements) {
//                 auto j = elem.variable_index;
//                 if (elem.factor < 0)
//                     C[i].push_back(j);
//                 if (elem.factor)
//                     I[i].push_back(j);
//                 r[i].emplace_back(0.0, j);
//             }

//             ++i;
//         }

//         Ensures(i == m, "make_c_i i != n");
//     }

//     void update_row(int k)
//     {
//         printf("update_row %d / %zu\n", k, I.size());
//         for (auto i : I[k])
//             P(k, i) *= theta;

//         for (index i = 0, endi = numeric_cast<index>(I[k].size());
//              i != endi; ++i) {
//             double sum_a_pi {0};
//             double sum_a_p {0};

//             assert(k < numeric_cast<index>(I.size()));
//             assert(i < numeric_cast<index>(I[k].size()));

//             for (index h = 0; h != m; ++h) {
//                 if (A(h, I[k][i])) {
//                     sum_a_pi += A(h, I[k][i]) * pi(h);
//                     sum_a_p += A(h, I[k][i]) * P(h, I[k][i]);
//                 }
//             }

//             printf("%ld %f %f\n", I[k][i], sum_a_pi, sum_a_p);
//             std::cout << c << '\n';

//             r[k][i].value = c(I[k][i]) - sum_a_pi - sum_a_p;
//             r[k][i].id = I[k][i];
//         }

//         // negate reduced costs and coefficients of -1 coefficient.
//         for (auto i : C[k]) {
//             r[k][i].value = - r[k][i].value;
//             A(k, i) = - A(k, i);
//             P(k, i) = - P(k, i);
//         }

//         int a_ki_ui {0};
//         for (auto i : C[k])
//             a_ki_ui += A(k, i) * u(i);

//         b(k, 0) += a_ki_ui;
//         b(k, 1) += a_ki_ui;

//         details::sort_generalized(r[k].begin(), r[k].end(), tag());

//         // search lowerbound
//         index i = 0;
//         index lower = -1;
//         index upper = -1;

//         for (index endi = numeric_cast<index>(r[k].size()); i != endi; ++i)
//         {
//             if (r[k][i].value > b(k, 0))
//                 break;
//             lower = i;
//         }

//         if (lower >= 0) {
//             for (index endi = numeric_cast<index>(r[k].size()); i != endi;
//             ++i) {
//                 if (r[k][i].value > 0 or r[k][i].value < b(k, 1))
//                     break;
//                 upper = i;
//             }

//             if (upper > 0) {
//                 // need to test which binary variables
//                 // 0 1 + 1 + 1 + ... = u
//             }
//         }

//         // double d = delta
//         //     + ((kappa / (1.0 - kappa))
//         //             * (std::get<double>(r[b(k)])
//         //                - std::get<double>(r[b(k) - 1])));

//         // for (index j = 0; j < b(k); ++j) {
//         //     x(std::get<index>(r[j])) = 1;
//         //     P(k, std::get<index>(r[j])) += d;
//         // }

//         // for (index j = b(k), endj = numeric_cast<index>(I[k].size());
//         //      j != endj; ++j) {
//         //     x(std::get<index>(r[j])) = 0;
//         //     P(k, std::get<index>(r[j])) -= d;
//         // }

//         // undo adjustment of row lower and upper bound
//         b(k, 0) += a_ki_ui;
//         b(k, 1) += a_ki_ui;

//         // clean up: correct negated costs and adjust value of negated
//         // variables.
//         for (auto i : C[k]) {
//             A(k, i) = - A(k, i);
//             P(k, i) = - P(k, i);
//             x(i) = u(i) - x(i);
//         }
//     }

// public:
//     generalized_wedelin(double kappa_, double delta_, double theta_,
//                         long limit, const problem& pb_)
//         : m(numeric_cast<index>(pb_.equal_constraints.size())
//             + numeric_cast<index>(pb_.greater_equal_constraints.size())
//             + numeric_cast<index>(pb_.less_equal_constraints.size()))
//         , n(numeric_cast<index>(pb_.vars.values.size()))
//         , pb(adapt_problem(pb_))
//         , A(make_inequality_a(m, n, pb))
//         , b(make_inequality_b(m, pb))
//         , c(make_generalized_c(n, pb))
//         , x(Eigen::VectorXi::Zero(n))
//         , u(Eigen::VectorXi::Zero(n))
//         , P(Eigen::MatrixXf::Zero(m, n))
//         , pi(Eigen::VectorXf::Zero(m))
//         , I(m)
//         , r(m)
//         , kappa(kappa_)
//         , delta(delta_)
//         , theta(theta_)
//         , loop(0)
//         , optimal(false)
//     {
//         Ensures(kappa >= 0 and kappa < 1, "kappa [0, 1[");
//         Ensures(delta >= 0, "delta [0, +oo[");
//         Ensures(theta >= 0 and theta <= 1, "theta [0, 1]");
//         Ensures(m > 0, "constraints number must be > 0");
//         Ensures(n > 0, "variable number must be > 0");

//         for (const auto& elem : pb.objective.elements)
//             x(elem.variable_index) = c(elem.variable_index) <= 0;

//         // build the C_k, I_k vectors store -1/1 and -1 coefficient.
//         make_c_i_r();

//         std::vector<index> R;
//         while (loop != limit) {
//             for (index k {0}; k != m; ++k) {
//                 int v = 0;
//                 for (index i {0}; i != n; ++i)
//                     v += A(k, i) * x(i);

//                 if (not (b(0, k) <= v and v <= b(1, k)))
//                     R.push_back(k);
//             }

//             if (R.empty()) {
//                 optimal = true;
//                 return;
//             }

//             for (auto k : R)
//                 update_row(k);

//             R.clear();
//             ++loop;
//         }
//     }

//     double compute_value() const
//     {
//         double ret {0};

//         for (auto& elem : pb.objective.elements)
//             ret += elem.factor * x(elem.variable_index);

//         return ret;
//     }

//     result results() const
//     {
//         result ret;
//         ret.loop = loop;
//         ret.optimal = optimal;
//         ret.value = compute_value();
//         ret.variable_name.resize(n);
//         ret.variable_value.resize(n, 0);

//         for (index i = 0; i != n; ++i) {
//             ret.variable_name[i] = pb.vars.names[i];
//             if (x(i))
//                 ret.variable_value[i] = true;
//         }

//         return ret;
//     }
// };

// } // namespace details

// inline
// result
// generalized_wedelin(double kappa, double delta, double theta,
//                     long limit, const problem& pb)
// {
//     using maximize_solver = details::generalized_wedelin<
//         details::maximize_generalized_tag>;
//     using minimize_solver = details::generalized_wedelin<
//         details::minimize_generalized_tag>;

//     if (pb.type == lp::objective_function_type::maximize) {
//         maximize_solver solver(kappa, delta, theta, limit, pb);
//         return solver.results();
//     }

//     minimize_solver solver(kappa, delta, theta, limit, pb);
//     return solver.results();
// }

// } // namespace lp

#endif
