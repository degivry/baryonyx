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

#include "unit-test.hpp"
#include <fstream>
#include <iostream>
#include <lpcore-compare>
#include <lpcore-out>
#include <lpcore>
#include <map>
#include <numeric>
#include <random>
#include <sstream>

void
test_assignment_problem(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/assignment_problem_1.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 50l;

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_assignment_problem_random_coast(std::shared_ptr<lp::context> ctx)
{
    std::map<std::string, lp::parameter> params;
    params["limit"] = 1000000l;
    params["theta"] = 0.5;
    params["delta"] = 0.2;
    params["kappa-step"] = 10e-4;
    params["kappa-max"] = 10.0;
    params["alpha"] = 0.0;
    params["w"] = 20l;

    for (int i{ 0 }, e{ 10 }; i != e; ++i) {
        auto pb =
          lp::make_problem(ctx, EXAMPLES_DIR "/assignment_problem_1.lp");

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);

        for (auto& elem : pb.objective.elements)
            elem.factor = dis(gen);

        auto result = lp::solve(ctx, pb, params);

        Ensures(result.status == lp::result_status::success);
        Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
    }
}

void
test_negative_coeff(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/negative-coeff.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 50l;

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_negative_coeff2(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/negative-coeff2.lp");

    std::map<std::string, lp::parameter> params;
    // params["constraint-order"] = std::string("random");
    params["limit"] = 2l;
    // params["theta"] = 0.5;
    // params["delta"] = 0.05;
    // params["kappa-step"] = 10e-4;
    // params["kappa-max"] = 10.0;
    // params["alpha"] = 0.0;
    // params["w"] = 60l;
    params["serialize"] = 1l;

    auto result = lp::solve(ctx, pb, params);

    for (auto elen : result.variable_value)
        std::cout << elen << ' ';
    std::cout << '\n';

    Ensures(result.status == lp::result_status::success);
    Ensures(result.variable_value[0] == 1);
    Ensures(result.variable_value[1] == 0);
    Ensures(result.variable_value[2] == 0);
    Ensures(result.variable_value[3] == 1);

    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_negative_coeff3(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/negative-coeff3.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 50l;

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_negative_coeff4(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/negative-coeff4.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 50l;

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_flat30_7(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/flat30-7.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000l;
    params["delta"] = 0.001;
    params["kappa-min"] = 0.3;
    params["kappa-step"] = 1e-4;
    params["kappa-max"] = 10.0;
    // params["alpha"] = 1.0;
    params["w"] = 60l;

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_uf50_0448(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/uf50-0448.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000'000l;
    params["theta"] = 0.5;
    params["delta"] = 0.000125;
    params["kappa-min"] = 0.1;
    params["kappa-step"] = 1e-4;
    params["kappa-max"] = 60.0;
    params["alpha"] = 1.0;
    params["w"] = 60l;
    params["constraint-order"] = std::string("none");

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_aim_50_1_6_yes1_2(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/aim-50-1_6-yes1-2.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000'000l;
    params["theta"] = 0.6;
    params["delta"] = 0.01;
    params["kappa-step"] = 2 * 10e-4;
    params["kappa-max"] = 100.0;
    params["alpha"] = 1.0;
    params["w"] = 20l;

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_bibd1n(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/bibd1n.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000'000l;
    params["theta"] = 0.5;
    params["delta"] = 9.5e-3; // 9e-3; //7.e-2;
    params["kappa-step"] = 1e-2;
    params["kappa-min"] = 0.65; // 0.7;
    params["kappa-max"] = 600.0;
    params["alpha"] = 0.0;
    params["w"] = 60l;
    params["serialize"] = 1l;
    params["constraint-order"] = std::string("none");
    // params["constraint-order"] = std::string("infeasibility-incr");
    // params["constraint-order"] = std::string("adaptative");
    // params["constraint-order"] = std::string("");
    params["time-limit"] = -100.0;

    auto result = lp::solve(ctx, pb, params);

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_8_queens_puzzle_fixed_cost(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/8_queens_puzzle.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000l;
    params["theta"] = 0.5;
    params["delta"] = 0.02;
    params["kappa-step"] = 0.01;
    params["kappa-max"] = 60.0;
    params["alpha"] = 1.0;
    params["w"] = 40l;

    std::vector<int> cost{ 25, 89, 12, 22, 84, 3,  61, 14, 93, 97, 68, 5,  51,
                           72, 96, 80, 13, 38, 81, 48, 70, 50, 66, 68, 30, 97,
                           79, 4,  41, 44, 47, 62, 60, 11, 18, 44, 57, 24, 7,
                           11, 66, 87, 9,  17, 27, 60, 95, 45, 94, 47, 60, 87,
                           79, 53, 81, 52, 91, 53, 57, 8,  63, 78, 1,  8 };

    std::size_t i{ 0 };

    for (auto& elem : pb.objective.elements)
        elem.factor = cost[i++];

    auto result = lp::solve(ctx, pb, params);

    for (int i = 0; i != 8; ++i) {
        for (int j = 0; j != 8; ++j) {
            std::cout << result.variable_value[j * 8 + i] << ' ';
        }
        std::cout << '\n';
    }

    Ensures(result.status == lp::result_status::success);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

void
test_8_queens_puzzle_random_cost(std::shared_ptr<lp::context> ctx)
{
    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000l;
    params["theta"] = 0.5;
    params["delta"] = 0.02;
    params["kappa-step"] = 0.01;
    params["kappa-max"] = 60.0;
    params["alpha"] = 1.0;
    params["w"] = 40l;
    params["constraint-order"] = std::string("infeasibility-decr");
    params["preprocessing"] = std::string("variables-weight");

    for (int i{ 0 }, e{ 10 }; i != e; ++i) {
        auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/8_queens_puzzle.lp");

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);

        for (auto& elem : pb.objective.elements)
            elem.factor = dis(gen);

        auto result = lp::solve(ctx, pb, params);

        Ensures(result.status == lp::result_status::success);
        Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
    }
}

void
test_qap(std::shared_ptr<lp::context> ctx)
{
    auto pb = lp::make_problem(ctx, EXAMPLES_DIR "/small4.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000l;
    params["theta"] = 0.5;
    params["delta"] = 0.2;
    params["kappa-step"] = 10e-4;
    params["kappa-max"] = 10.0;
    params["alpha"] = 0.0;
    params["w"] = 20l;

    auto result = lp::solve(ctx, pb, params);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
}

int
main(int /* argc */, char* /* argv */ [])
{
    auto ctx = std::make_shared<lp::context>();
    ctx->set_standard_stream_logger();

    test_assignment_problem(ctx);
    test_assignment_problem_random_coast(ctx);
    test_negative_coeff(ctx);
    test_negative_coeff2(ctx);
    test_negative_coeff3(ctx);
    test_negative_coeff4(ctx);
    test_8_queens_puzzle_fixed_cost(ctx);
    test_8_queens_puzzle_random_cost(ctx);
    test_qap(ctx);
    test_uf50_0448(ctx);
    test_flat30_7(ctx);
    test_aim_50_1_6_yes1_2(ctx);

    // test_bibd1n(ctx);

    return unit_test::report_errors();
}
