#pragma once

#include <entt/core/family.hpp>

namespace osp
{

//using machine_family_t = entt::family<struct resource_type>;
//using machine_id_t = machine_family_t::family_type;
using machine_id_t = uint32_t;

struct RegisteredMachine
{
    RegisteredMachine(machine_id_t id) : m_id(id) {}

    machine_id_t m_id;

    // TODO: possibly put some info on fields for the config parser
};

}
