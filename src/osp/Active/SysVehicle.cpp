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
#include "ActiveScene.h"
#include "SysVehicle.h"
#include "SysDebugRender.h"
#include "physics.h"

#include "../Satellites/SatActiveArea.h"
#include "../Satellites/SatVehicle.h"
#include "../Resource/PrototypePart.h"
#include "../Resource/AssetImporter.h"
#include "adera/Shaders/Phong.h"

#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>

#include <iostream>


using namespace osp;
using namespace osp::active;

using osp::universe::Universe;
using osp::universe::Satellite;
using osp::universe::UCompVehicle;

// for the 0xrrggbb_rgbf literalsm
using namespace Magnum::Math::Literals;

const std::string SysVehicle::smc_name = "Vehicle";

SysVehicle::SysVehicle(ActiveScene &scene)
 : m_updateActivation(
       scene.get_update_order(), "vehicle_activate", "", "vehicle_modification",
       &SysVehicle::update_activate)
 , m_updateVehicleModification(
       scene.get_update_order(), "vehicle_modification", "", "physics",
       &SysVehicle::update_vehicle_modification)
{ }


ActiveEnt SysVehicle::activate(ActiveScene &rScene, universe::Universe &rUni,
                          universe::Satellite areaSat,
                          universe::Satellite tgtSat)
{


    std::cout << "loadin a vehicle!\n";

    auto &loadMeVehicle = rUni.get_reg().get<universe::UCompVehicle>(tgtSat);
    auto &tgtPosTraj = rUni.get_reg().get<universe::UCompTransformTraj>(tgtSat);

    // make sure there is vehicle data to load
    if (loadMeVehicle.m_blueprint.empty())
    {
        // no vehicle data to load
        return entt::null;
    }

    BlueprintVehicle &vehicleData = *(loadMeVehicle.m_blueprint);
    ActiveEnt root = rScene.hier_get_root();

    // Create the root entity to add parts to

    ActiveEnt vehicleEnt = rScene.hier_create_child(root, "Vehicle");

    rScene.reg_emplace<ACompActivatedSat>(vehicleEnt, tgtSat);

    ACompVehicle& vehicleComp = rScene.reg_emplace<ACompVehicle>(vehicleEnt);

    // Convert position of the satellite to position in scene
    Vector3 positionInScene = rUni.sat_calc_pos_meters(areaSat, tgtSat);

    ACompTransform& vehicleTransform = rScene.get_registry()
                                        .emplace<ACompTransform>(vehicleEnt);
    vehicleTransform.m_transform
            = Matrix4::from(tgtPosTraj.m_rotation.toMatrix(), positionInScene);
    rScene.reg_emplace<ACompFloatingOrigin>(vehicleEnt);
    //vehicleTransform.m_enableFloatingOrigin = true;

    // Create the parts

    // Unique part prototypes used in the vehicle
    // Access with [blueprintParts.m_partIndex]
    std::vector<DependRes<PrototypePart> >& partsUsed =
            vehicleData.get_prototypes();

    // All the parts in the vehicle
    std::vector<BlueprintPart> &blueprintParts = vehicleData.get_blueprints();

    // Keep track of parts
    //std::vector<ActiveEnt> newEntities;
    vehicleComp.m_parts.reserve(blueprintParts.size());

    // Loop through list of blueprint parts
    for (BlueprintPart& partBp : blueprintParts)
    {
        DependRes<PrototypePart>& partDepends =
                partsUsed[partBp.m_protoIndex];

        // Check if the part prototype this depends on still exists
        if (partDepends.empty())
        {
            return entt::null;
        }

        PrototypePart &proto = *partDepends;

        ActiveEnt partEntity = part_instantiate(rScene, proto,
                        partBp, vehicleEnt);

        vehicleComp.m_parts.push_back(partEntity);

        auto& partPart = rScene.reg_emplace<ACompPart>(partEntity);
        partPart.m_vehicle = vehicleEnt;

        // Part now exists

        ACompTransform& partTransform = rScene.get_registry()
                                            .get<ACompTransform>(partEntity);

        // set the transformation
        partTransform.m_transform
                = Matrix4::from(partBp.m_rotation.toMatrix(),
                              partBp.m_translation)
                * Matrix4::scaling(partBp.m_scale);

        // temporary: initialize the rigid body
        //area.get_scene()->debug_get_newton().create_body(partEntity);
    }

    // Wire the thing up

    #if 0
    SysWire& sysWire = rScene.dynamic_system_find<SysWire>();

    // Loop through wire connections
    for (BlueprintWire& blueprintWire : vehicleData.get_wires())
    {
        // TODO: check if the connections are valid

        // get wire from

        ACompMachines& fromMachines = rScene.reg_get<ACompMachines>(
                *(vehicleComp.m_parts.begin() + blueprintWire.m_fromPart));

        auto fromMachineEntry = fromMachines
                .m_machines[blueprintWire.m_fromMachine];
        Machine &fromMachine = fromMachineEntry.m_system->second
                ->get(fromMachineEntry.m_partEnt);
        WireOutput* fromWire =
                fromMachine.request_output(blueprintWire.m_fromPort);

        // get wire to

        ACompMachines& toMachines = rScene.reg_get<ACompMachines>(
                *(vehicleComp.m_parts.begin() + blueprintWire.m_toPart));

        auto toMachineEntry = toMachines
                .m_machines[blueprintWire.m_toMachine];
        Machine &toMachine = toMachineEntry.m_system->second
                ->get(toMachineEntry.m_partEnt);
        WireInput* toWire =
                toMachine.request_input(blueprintWire.m_toPort);

        // make the connection

        sysWire.connect(*fromWire, *toWire);
    }
    #endif

    // temporary: make the whole thing a single rigid body
    auto& vehicleBody = rScene.reg_emplace<ACompRigidBody_t>(vehicleEnt);
    rScene.reg_emplace<ACompShape>(vehicleEnt, phys::ECollisionShape::COMBINED);
    rScene.reg_emplace<ACompCollider>(vehicleEnt, nullptr);
    //scene.dynamic_system_find<SysPhysics>().create_body(vehicleEnt);

    return vehicleEnt;

    return entt::null;
}

#if 0
void SysVehicle::part_instantiate_machines(ActiveScene& rScene, ActiveEnt partEnt,
    std::vector<MachineDef> const& machineMapping,
    PrototypePart const& part, BlueprintPart const& partBP)
{
    std::vector<PrototypeMachine> const& protoMachines = part.get_machines();
    std::vector<BlueprintMachine> const& bpMachines = partBP.m_machines;
    for (auto const& obj : machineMapping)
    {
        add_machines_to_object(rScene, partEnt, obj.m_machineOwner,
            protoMachines, bpMachines, obj.m_machineIndices);
    }
}
#endif

// Traverses the hierarchy and sums the volume of all ACompShapes it finds
float SysVehicle::compute_hier_volume(ActiveScene& rScene, ActiveEnt part)
{
    float volume = 0.0f;
    auto checkVol = [&rScene, &volume](ActiveEnt ent)
    {
        auto const* shape = rScene.get_registry().try_get<ACompShape>(ent);
        if (shape)
        {
            auto const& xform = rScene.reg_get<ACompTransform>(ent);
            volume += shape_volume(shape->m_shape, xform.m_transform.scaling());
        }
        return EHierarchyTraverseStatus::Continue;
    };

    rScene.hierarchy_traverse(part, std::move(checkVol));

    return volume;
}

ActiveEnt SysVehicle::part_instantiate(
    ActiveScene& rScene, PrototypePart const& part, BlueprintPart const& blueprint, ActiveEnt rootParent)
{
    // TODO: make some changes to ActiveScene for creating multiple entities easier

    std::vector<ActiveEnt> newEntities(part.m_entityCount);
    ActiveEnt& rootEntity = newEntities[0];

    // reserve space for new entities and ACompTransforms to be created
    rScene.get_registry().reserve(
                rScene.get_registry().capacity() + part.m_entityCount);
    rScene.get_registry().reserve<ACompTransform>(
                rScene.get_registry().capacity<ACompTransform>() + part.m_entityCount);

    // Create entities and hierarchy
    for (size_t i = 0; i < part.m_entityCount; i++)
    {
        PCompHierarchy const &pcompHier = part.m_partHier[i];
        ActiveEnt parentEnt = entt::null;

        // Get parent
        if (pcompHier.m_parent == i)
        {
            // if parented to self (the root)
            parentEnt = rootParent;//m_scene->hier_get_root();
        }
        else
        {
            // since objects were loaded recursively, the parents always load
            // before their children
            parentEnt = newEntities[pcompHier.m_parent];
        }

        // Create the new entity
        ActiveEnt currentEnt = rScene.hier_create_child(parentEnt);
        newEntities[i] = currentEnt;

        // Add and set transform component
        PCompTransform const &pcompTransform = part.m_partTransform[i];
        auto &rCurrentTransform = rScene.reg_emplace<ACompTransform>(currentEnt);
        rCurrentTransform.m_transform
                = Matrix4::from(pcompTransform.m_rotation.toMatrix(),
                                pcompTransform.m_translation)
                * Matrix4::scaling(pcompTransform.m_scale);
    }

    // TODO: add an ACompName because putting name in hierarchy is pretty stupid
    for (PCompName const& pcompName : part.m_partName)
    {
        auto &hier = rScene.reg_get<ACompHierarchy>(newEntities[pcompName.m_entity]);
        hier.m_name = pcompName.m_name;
    }

    rScene.get_registry().reserve<CompDrawableDebug>(
                rScene.get_registry().capacity<CompDrawableDebug>()
                + part.m_partDrawable.size());

    // Create drawables
    for (PCompDrawable const& pcompDrawable : part.m_partDrawable)
    {
        using Magnum::GL::Mesh;
        using Magnum::Trade::MeshData;
        using Magnum::GL::Texture2D;
        using Magnum::Trade::ImageData2D;

        Package& package = rScene.get_application().debug_find_package("lzdb");

        Package& glResources = rScene.get_context_resources();
        DependRes<MeshData> meshData = pcompDrawable.m_mesh;
        DependRes<Mesh> meshRes = glResources.get<Mesh>(meshData.name());

        if (meshRes.empty())
        {
            // Mesh isn't compiled yet, compile it
            meshRes = AssetImporter::compile_mesh(meshData, glResources);
        }

        std::vector<DependRes<Texture2D>> textureResources;
        textureResources.reserve(pcompDrawable.m_textures.size());
        for (DependRes<ImageData2D> imageData : pcompDrawable.m_textures)
        {
            DependRes<Texture2D> texRes = glResources.get<Texture2D>(imageData.name());

            if (texRes.empty())
            {
                // Texture isn't compiled yet, compile it
                texRes = AssetImporter::compile_tex(imageData, glResources);
            }
            textureResources.push_back(texRes);
        }

        // by now, the mesh and texture should both exist

        ActiveEnt currentEnt = newEntities[pcompDrawable.m_entity];

        // TODO: Create components for generic properties of drawables:
        //       ACompMesh, ACompSolid, ACompRoughnessTexture...

        using adera::shader::Phong;
        auto &shader = rScene.reg_emplace<Phong::ACompPhongInstance>(currentEnt);
        shader.m_shaderProgram = glResources.get<Phong>("phong_shader");
        shader.m_textures = std::move(textureResources);
        shader.m_lightPosition = Vector3{10.0f, 15.0f, 5.0f};
        shader.m_ambientColor = 0x111111_rgbf;
        shader.m_specularColor = 0x330000_rgbf;

        rScene.reg_emplace<CompDrawableDebug>(
                currentEnt, meshRes, &Phong::draw_entity, 0x0202EE_rgbf);
    }

    rScene.get_registry().reserve<PCompPrimativeCollider>(
                rScene.get_registry().capacity<PCompPrimativeCollider>()
                + part.m_partCollider.size());

    // Create primative colliders
    for (PCompPrimativeCollider const& pcompCollider : part.m_partCollider)
    {
        ActiveEnt currentEnt = newEntities[pcompCollider.m_entity];
        ACompShape& collision = rScene.reg_emplace<ACompShape>(currentEnt);
        rScene.reg_emplace<ACompCollider>(currentEnt);
        collision.m_shape = pcompCollider.m_shape;
    }

    // TODO: individual glTF nodes can now have masses, but there's no
    //       implementation for it yet. This is a workaround to keep the old
    //       system

    // Create masses
    float totalMass = 0;
    for (PCompMass const& pcompMass : part.m_partMass)
    {
        totalMass += pcompMass.m_mass;
    }

    float partVolume = compute_hier_volume(rScene, rootEntity);
    float partDensity = totalMass / partVolume;

    auto applyMasses = [&rScene, partDensity](ActiveEnt ent)
    {
        if (auto const* shape = rScene.reg_try_get<ACompShape>(ent);
            shape != nullptr)
        {
            if (!rScene.get_registry().has<ACompCollider>(ent))
            {
                return EHierarchyTraverseStatus::Continue;
            }
            Matrix4 const& transform = rScene.reg_get<ACompTransform>(ent).m_transform;
            float volume = phys::shape_volume(shape->m_shape, transform.scaling());
            float mass = volume * partDensity;

            rScene.reg_emplace<ACompMass>(ent, mass);
        }
        return EHierarchyTraverseStatus::Continue;
    };

    rScene.hierarchy_traverse(rootEntity, applyMasses);

    return rootEntity;
}

void SysVehicle::update_activate(ActiveScene &rScene)
{
    ACompAreaLink *pArea = SysAreaAssociate::try_get_area_link(rScene);

    if (pArea == nullptr)
    {
        return;
    }

    Universe &rUni = pArea->get_universe();

    // Delete vehicles that have gone too far from the ActiveArea range
    for (auto const &[sat, ent] : pArea->m_leave)
    {
        if (!rUni.get_reg().has<UCompVehicle>(sat))
        {
            continue;
        }

        rScene.hier_destroy(ent);
    }

    // Update transforms of already-activated vehicle satellites
    auto view = rScene.get_registry().view<ACompVehicle, ACompTransform,
                                           ACompActivatedSat>();
    for (ActiveEnt vehicleEnt : view)
    {
        auto &vehicleTf = view.get<ACompTransform>(vehicleEnt);
        auto &vehicleSat = view.get<ACompActivatedSat>(vehicleEnt);
        SysAreaAssociate::sat_transform_set_relative(
            rUni, pArea->m_areaSat, vehicleSat.m_sat, vehicleTf.m_transform);
    }

    // Activate nearby vehicle satellites that have just entered the ActiveArea
    for (auto &entered : pArea->m_enter)
    {
        universe::Satellite sat = entered->first;
        ActiveEnt &rEnt = entered->second;

        if (!rUni.get_reg().has<UCompVehicle>(sat))
        {
            continue;
        }

        rEnt = activate(rScene, rUni, pArea->m_areaSat, sat);
    }
}

void SysVehicle::update_vehicle_modification(ActiveScene& rScene)
{
    auto view = rScene.get_registry().view<ACompVehicle>();
    auto viewParts = rScene.get_registry().view<ACompPart>();

    // this part is sort of temporary and unoptimized. deal with it when it
    // becomes a problem. TODO: use more views

    for (ActiveEnt vehicleEnt : view)
    {
        auto &vehicleVehicle = view.get<ACompVehicle>(vehicleEnt);
        //std::vector<ActiveEnt> &parts = vehicleVehicle.m_parts;

        if (vehicleVehicle.m_separationCount > 0)
        {
            // Separation requested

            // mark collider as dirty
            auto &rVehicleBody = rScene.reg_get<ACompRigidBody_t>(vehicleEnt);
            rVehicleBody.m_colliderDirty = true;

            // Invalidate all ACompRigidbodyAncestors
            auto invalidateAncestors = [&rScene](ActiveEnt e)
            {
                auto* pRBA = rScene.get_registry().try_get<ACompRigidbodyAncestor>(e);
                if (pRBA) { pRBA->m_ancestor = entt::null; }
                return EHierarchyTraverseStatus::Continue;
            };
            rScene.hierarchy_traverse(vehicleEnt, invalidateAncestors);

            // Create the islands vector
            // [0]: current vehicle
            // [1+]: new vehicles
            std::vector<ActiveEnt> islands(vehicleVehicle.m_separationCount);
            vehicleVehicle.m_separationCount = 0;

            islands[0] = vehicleEnt;

            // NOTE: vehicleVehicle and vehicleTransform become invalid
            //       when emplacing new ones.
            for (size_t i = 1; i < islands.size(); i ++)
            {
                ActiveEnt islandEnt = rScene.hier_create_child(
                            rScene.hier_get_root());
                rScene.reg_emplace<ACompVehicle>(islandEnt);
                auto &islandTransform
                        = rScene.reg_emplace<ACompTransform>(islandEnt);
                auto &islandBody
                        = rScene.reg_emplace<ACompRigidBody_t>(islandEnt);
                auto &islandShape
                        = rScene.reg_emplace<ACompShape>(islandEnt);
                auto &islandCollider
                        = rScene.reg_emplace<ACompCollider>(islandEnt, nullptr);
                islandShape.m_shape = phys::ECollisionShape::COMBINED;

                auto &vehicleTransform = rScene.reg_get<ACompTransform>(vehicleEnt);
                islandTransform.m_transform = vehicleTransform.m_transform;
                rScene.reg_emplace<ACompFloatingOrigin>(islandEnt);

                islands[i] = islandEnt;
            }


            // iterate through parts
            // * remove parts that are destroyed, destroy the part entity too
            // * remove parts different islands, and move them to the new
            //   vehicle


            entt::basic_registry<ActiveEnt> &reg = rScene.get_registry();
            auto removeDestroyed = [&viewParts, &rScene, &islands]
                    (ActiveEnt partEnt) -> bool
            {
                auto &partPart = viewParts.get<ACompPart>(partEnt);
                if (partPart.m_destroy)
                {
                    // destroy this part
                    rScene.hier_destroy(partEnt);
                    return true;
                }

                if (partPart.m_separationIsland)
                {
                    // separate into a new vehicle

                    ActiveEnt islandEnt = islands[partPart.m_separationIsland];
                    auto &islandVehicle = rScene.reg_get<ACompVehicle>(
                                islandEnt);
                    islandVehicle.m_parts.push_back(partEnt);

                    rScene.hier_set_parent_child(islandEnt, partEnt);

                    return true;
                }

                return false;

            };

            std::vector<ActiveEnt> &parts
                    = view.get<ACompVehicle>(vehicleEnt).m_parts;

            parts.erase(std::remove_if(parts.begin(), parts.end(),
                                       removeDestroyed), parts.end());

            // update or create rigid bodies. also set center of masses

            for (ActiveEnt islandEnt : islands)
            {
                auto &islandVehicle = view.get<ACompVehicle>(islandEnt);
                auto &islandTransform
                        = rScene.reg_get<ACompTransform>(islandEnt);

                Vector3 comOffset;
                //float totalMass;

                for (ActiveEnt partEnt : islandVehicle.m_parts)
                {
                    // TODO: deal with part mass
                    auto const &partTransform
                            = rScene.reg_get<ACompTransform>(partEnt);

                    comOffset += partTransform.m_transform.translation();

                }

                comOffset /= islandVehicle.m_parts.size();



                for (ActiveEnt partEnt : islandVehicle.m_parts)
                {
                    ActiveEnt child = rScene.reg_get<ACompHierarchy>(partEnt)
                                        .m_childFirst;
                    auto &partTransform
                            = rScene.reg_get<ACompTransform>(partEnt);

                    partTransform.m_transform.translation() -= comOffset;
                }

//                ActiveEnt child = m_scene.reg_get<ACompHierarchy>(islandEnt)
//                                    .m_childFirst;
//                while (m_scene.get_registry().valid(child))
//                {
//                    auto &childTransform
//                            = m_scene.reg_get<ACompTransform>(child);

//                    childTransform.m_transform.translation() -= comOffset;

//                    child = m_scene.reg_get<ACompHierarchy>(child)
//                            .m_siblingNext;
//                }

                islandTransform.m_transform.translation() += comOffset;

                //m_scene.dynamic_system_find<SysPhysics>().create_body(islandEnt);
            }

        }
    }
}

