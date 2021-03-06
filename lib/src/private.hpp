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

#ifndef ORG_VLEPROJECT_BARYONYX_SOLVER_PRIVATE_HPP
#define ORG_VLEPROJECT_BARYONYX_SOLVER_PRIVATE_HPP

#include <baryonyx/core>

namespace baryonyx {

struct maximize_tag
{};

struct minimize_tag
{};

} // namespace baryonyx

namespace baryonyx_private {

baryonyx::problem
read_problem(std::istream& is);

baryonyx::result
read_result(std::istream& is);

bool
write_problem(std::ostream& os, const baryonyx::problem& pb);

bool
check_consistency(const baryonyx::problem& pb);

void
preprocess(const std::shared_ptr<baryonyx::context>& ctx,
           baryonyx::problem& pb);

baryonyx::result
solve(const std::shared_ptr<baryonyx::context>& ctx, baryonyx::problem& pb);

baryonyx::result
optimize(std::shared_ptr<baryonyx::context> ctx, baryonyx::problem& pb);

} // namespace baryonyx

#endif
