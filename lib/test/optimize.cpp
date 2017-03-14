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
test_qap()
{
    auto pb = lp::make_problem(EXAMPLES_DIR "/small4.lp");

    std::map<std::string, lp::parameter> params;
    params["limit"] = 10'000'000l;
    params["theta"] = 0.5;
    params["delta"] = 0.2;
    params["kappa-step"] = 10e-4;
    params["kappa-max"] = 10.0;
    params["alpha"] = 2.0;
    params["w"] = 20l;

    params["pushing-k-factor"] = 0.9;
    params["pushes-limit"] = 100l;
    params["pushing-objective-amplifier"] = 5l;
    params["pushing-iteration-limit"] = 50l;

    auto result = lp::optimize(pb, params);
    Ensures(lp::is_valid_solution(pb, result.variable_value) == true);
    Ensures(lp::compute_solution(pb, result.variable_value) == 790.0);
}

int
main(int /* argc */, char* /* argv */ [])
{
    test_qap();

    return unit_test::report_errors();
}
