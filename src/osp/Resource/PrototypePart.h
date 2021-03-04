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

#include <string>
#include <array>
#include <vector>
#include <variant>

#include "../types.h"
#include "osp/CommonPhysics.h"

namespace osp
{

using PartEntity_t = uint32_t;
using config_node_t = std::variant<double, int, std::string>;

// Required components

struct PCompHierarchy
{
    PartEntity_t m_parent;
    uint16_t m_childCount;
};

struct PCompTransform
{
    Magnum::Vector3 m_translation;
    Magnum::Quaternion m_rotation;
    Magnum::Vector3 m_scale;
};

// Optional components

struct PCompDrawable
{
    PartEntity_t m_entity;
    uint32_t m_mesh;
    std::vector<uint32_t> m_textures;
};

struct PCompCollider
{
    PartEntity_t m_entity;
    phys::ECollisionShape m_type;
    unsigned m_meshData;
};

struct PCompMass
{
    PartEntity_t m_entity;
    float m_mass;
};

struct PCompName
{
    PartEntity_t m_entity;
    std::string m_name;
};

struct PCompMachine
{
    PartEntity_t m_entity;
    std::string m_type;
    std::map<std::string, config_node_t> m_config;
};

/**
 * Describes everything on how to construct a part, loaded directly from a file
 * or something
 */
struct PrototypePart
{

    PartEntity_t m_entityCount;

    // Parallel
    std::vector<PCompHierarchy> m_partHier;
    std::vector<PCompTransform> m_partTransform;

    // Optional
    std::vector<PCompDrawable> m_partDrawable;
    std::vector<PCompCollider> m_partCollider;
    std::vector<PCompMass> m_partMass;
    std::vector<PCompName> m_partName;
    std::vector<PCompMachine> m_partMachines;
};

}
