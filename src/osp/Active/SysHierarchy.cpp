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

#include "SysHierarchy.h"

using osp::active::SysHierarchy;
using osp::active::ActiveEnt;

void SysHierarchy::setup(ActiveScene& rScene)
{
    auto &rReg = rScene.get_registry();

    rReg.emplace<ACompHierarchy>(rScene.hier_get_root());
}

ActiveEnt SysHierarchy::create_child(ActiveScene& rScene, ActiveEnt parent,
                                            std::string const& name)
{
    auto &rReg = rScene.get_registry();
    ActiveEnt child = rReg.create();
    rReg.emplace<ACompHierarchy>(child);

    if (!name.empty())
    {
        rScene.reg_emplace<ACompName>(child, create_shared_string(name));
    }

    set_parent_child(rScene, parent, child);

    return child;
}

void SysHierarchy::set_parent_child(ActiveScene& rScene, ActiveEnt parent, ActiveEnt child)
{
    auto &rReg = rScene.get_registry();

    ACompHierarchy& childHierarchy = rReg.get<ACompHierarchy>(child);
    ACompHierarchy& parentHierarchy = rReg.get<ACompHierarchy>(parent);

    // if child has an existing parent, cut first
    if (rReg.valid(childHierarchy.m_parent))
    {
        cut(rScene, child);
    }

    // set new child's parent
    childHierarchy.m_parent = parent;
    childHierarchy.m_level = parentHierarchy.m_level + 1;

    // If has siblings (not first child)
    if (parentHierarchy.m_childCount)
    {
        ActiveEnt sibling = parentHierarchy.m_childFirst;
        auto& siblingHierarchy = rReg.get<ACompHierarchy>(sibling);

        // Set new child and former first child as siblings
        siblingHierarchy.m_siblingPrev = child;
        childHierarchy.m_siblingNext = sibling;
    }

    // Set parent's first child to new child just created
    parentHierarchy.m_childFirst = child;
    parentHierarchy.m_childCount ++; // increase child count
}

void SysHierarchy::cut(ActiveScene& rScene, ActiveEnt ent)
{
    auto &rReg = rScene.get_registry();
    auto &entHier = rReg.get<ACompHierarchy>(ent);

    // TODO: deal with m_depth

    // Unlink siblings by connecting previous and next to each other

    if (rReg.valid(entHier.m_siblingNext))
    {
        rReg.get<ACompHierarchy>(entHier.m_siblingNext).m_siblingPrev
                = entHier.m_siblingPrev;
    }

    if (rReg.valid(entHier.m_siblingPrev))
    {
        rReg.get<ACompHierarchy>(entHier.m_siblingPrev).m_siblingNext
                = entHier.m_siblingNext;
    }

    // Unlink parent

    auto &parentHier = rReg.get<ACompHierarchy>(entHier.m_parent);
    parentHier.m_childCount --;

    if (parentHier.m_childFirst == ent)
    {
        parentHier.m_childFirst = entHier.m_siblingNext;
    }

    entHier.m_level = 0;
    entHier.m_parent = entHier.m_siblingNext = entHier.m_siblingPrev
            = entt::null;
}

void SysHierarchy::sort(ActiveScene& rScene)
{
    rScene.get_registry().sort<ACompHierarchy>(
            [](ACompHierarchy const& lhs, ACompHierarchy const& rhs)
    {
        return lhs.m_level < rhs.m_level;
    }, entt::insertion_sort());
}


void SysHierarchy::update_delete(ActiveScene& rScene)
{
    auto &rReg = rScene.get_registry();
    auto viewDelHier = rReg.view<ACompDelete, ACompHierarchy>();

    // copy entities to delete into a buffer
    std::vector<ActiveEnt> toDelete;
    toDelete.reserve(viewDelHier.size_hint());
    toDelete.assign(std::begin(viewDelHier), std::end(viewDelHier));

    // Add delete components to descendents of all entities to delete
    for (ActiveEnt ent : toDelete)
    {
        traverse(rScene, ent, [&rScene] (ActiveEnt descendent) {
            rScene.mark_delete(descendent);
            return EHierarchyTraverseStatus::Continue;
        });
    }
}

