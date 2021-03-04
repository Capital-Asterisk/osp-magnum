/**
 * Open Space Program
 * Copyright © 2019-2020 Open Space Program Project
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

#include "SysWire.h"

#include <entt/core/family.hpp>

#include <cstdint>
#include <iostream>
#include <vector>

namespace osp::active
{



using Corrade::Containers::LinkedList;
using Corrade::Containers::LinkedListItem;

class ISysMachine;
class Machine;


//-----------------------------------------------------------------------------

/**
 * This component is added to a part, and stores a vector that keeps track of
 * all the Machines it uses. Machines are stored in multiple entities, so the
 * vector stores pairs of [Entity, Machine Type (iterator to system class)]
 */
struct ACompMachines
{

    ACompMachines() noexcept = default;
    ACompMachines(ACompMachines&& move) noexcept = default;
    ACompMachines(ACompMachines const& move) = delete;
    ACompMachines& operator=(ACompMachines&& move) = default;
    ACompMachines& operator=(ACompMachines const& move) = delete;

    //LinkedList<Machine> m_machines;
    //std::vector<PartMachine> m_machines;
    int dummy;
};

//-----------------------------------------------------------------------------


} // namespace osp::active
