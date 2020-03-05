#pragma once

#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Mesh.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>

#include <iostream>
#include <cstdint>
#include <string>

//#include "SturdyImporter.h"
#include "PartPrototype.h"

namespace osp
{

//class SturdyFile;
class PartPrototype;

//
// What I think might be a good idea:
//
// Packages cache, address, and maybe store all resources usable in the game
// They have names like "cool-rocket-engines-v2"
// They also have 4-character unique ASCII prefix to refer to them
//
// resources paths look like this:
// prfx:dirmaybe/resource
//
// note: dirs are kind of pointless
//
// TODO: Some sort of data-loading strategy/policy: folder, zip, network etc...
//       For now, packages only store loaded data
//
// # ignore comments below:
// PackageDir describe a directory in OSPData
// Package is abstracted because there might also have something like
// PackageZip or PackageNetwork (custom stuff from a multiplayer server)
// so packages don't strictly have to be folders.


struct AbstractResource
{

    //std::string m_name; // name is kind of useless when there's path
    std::string m_path;

    bool m_loaded;

    // TODO: * some sort of type identification, or just use typeof.
    //         implement when needed.
    //       * a way to identify loading strategy: dir, network, zip, etc
    //       * dependency to other resources

};

template <class T>
struct Resource : public AbstractResource
{
    Resource() = default;
    Resource(T&& move) : m_data(std::move(move)) {};
    Resource(const T& copy) : m_data(copy) {};

    T m_data;
};


// supported resources
struct ResourceTable  :

    std::vector< Resource<Magnum::GL::Mesh> >,
    std::vector< Resource<Magnum::Trade::MeshData3D> >,
    std::vector< Resource<Magnum::Trade::ImageData2D> >,
    std::vector< Resource<PartPrototype> >
    //std::vector< Resource<SturdyFile> >
{
    // prevent copying
    ResourceTable() = default;
    ResourceTable(ResourceTable&& move) = default;
    ResourceTable(const ResourceTable& copy) = delete;
};


class Package
{

public:

    Package(Package&& move) = default;
    Package(const Package& copy) = delete;
    //template <typename T>
    //struct TypeMap {  std::vector< T > fish; };

    ResourceTable m_resources;

    //typedef uint32_t Prefix;
    typedef char Prefix[4];

    Package(Prefix&& prefix, std::string const& packageName);

    //virtual Magnum::GL::Mesh* request_mesh(const std::string& path);

    //TypeMap<int>::fish;
    //std::vector< Resource<Magnum::Trade::ImageData> > g_imageData;

    template<class T>
    Resource<T>* debug_add_resource(Resource<T>&& resource);

    template<class T>
    Resource<T>* get_resource(std::string const& path);

    template<class T>
    Resource<T>* get_resource(unsigned resIndex);

private:
    // highly suggested to not change these when initialized, so const.
    const std::string m_packageName;

    const Prefix m_prefix;

    std::string m_displayName;

};

template<class T>
Resource<T>* Package::debug_add_resource(Resource<T>&& resource)
{
    std::vector< Resource<T> >& resourceVec =
            static_cast< std::vector< Resource<T> >& >(m_resources);

    resourceVec.push_back(std::move(resource));

    return &(resourceVec.back());

}


template<class T>
Resource<T>* Package::get_resource(std::string const& path)
{
    std::vector< Resource<T> >& resourceVec =
            static_cast< std::vector< Resource<T> >& >(m_resources);

    // just match the path for now, find a better way eventually

    for (Resource<T>& resource : resourceVec)
    {
        if (resource.m_path == path)
        {
            return &resource;
        }
    }
    return nullptr;
}


template<class T>
Resource<T>* Package::get_resource(unsigned resIndex)
{
    std::vector< Resource<T> >& resourceVec =
            static_cast< std::vector< Resource<T> >& >(m_resources);

    // check if the index is within the array size
    if (resIndex < resourceVec.size())
    {
        return &(resourceVec[resIndex]);
    }

    return nullptr;
}

}