//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018, Don Boogert. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of Don Boogert nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "GafferVDB/PlatonicLevelSet.h"
#include "GafferVDB/Interrupt.h"

#include "Gaffer/StringPlug.h"

#include "IECoreScene/MeshPrimitive.h"

#include "IECoreVDB/VDBObject.h"

#include "openvdb/tools/LevelSetPlatonic.h"


using namespace Gaffer;
using namespace GafferScene;
using namespace GafferVDB;
using namespace Imath;
using namespace IECore;
using namespace IECoreScene;
using namespace IECoreVDB;


IE_CORE_DEFINERUNTIMETYPED( PlatonicLevelSet );

size_t PlatonicLevelSet::g_firstPlugIndex = 0;

PlatonicLevelSet::PlatonicLevelSet( const std::string &name )
        :	ObjectSource( name, "vdb" )
{
    storeIndexOfNextChild( g_firstPlugIndex );
    addChild( new StringPlug( "grid", Plug::In, "surface") );
    addChild( new IntPlug( "faces", Plug::In, 4 ) );
    addChild( new FloatPlug( "scale", Plug::In, 1.0f ) );
    addChild( new V3fPlug( "center", Plug::In, V3f( 0.0f, 0.0f, 0.0f ) ) );
    addChild( new FloatPlug( "voxelSize", Plug::In, 0.1f ) );
    addChild( new FloatPlug( "halfWidth", Plug::In, (float) openvdb::LEVEL_SET_HALF_WIDTH ) );
}

PlatonicLevelSet::~PlatonicLevelSet()
{
}

Gaffer::StringPlug *PlatonicLevelSet::gridPlug()
{
    return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *PlatonicLevelSet::gridPlug() const
{
    return getChild<StringPlug>( g_firstPlugIndex );
}

Gaffer::IntPlug *PlatonicLevelSet::facesPlug()
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::IntPlug *PlatonicLevelSet::facesPlug() const
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

Gaffer::FloatPlug *PlatonicLevelSet::scalePlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::FloatPlug *PlatonicLevelSet::scalePlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

Gaffer::V3fPlug *PlatonicLevelSet::centerPlug()
{
    return getChild<V3fPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::V3fPlug *PlatonicLevelSet::centerPlug() const
{
    return getChild<V3fPlug>( g_firstPlugIndex + 3 );
}

Gaffer::FloatPlug *PlatonicLevelSet::voxelSizePlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 4 );
}

const Gaffer::FloatPlug *PlatonicLevelSet::voxelSizePlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 4 );
}

Gaffer::FloatPlug *PlatonicLevelSet::halfWidthPlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 5 );
}

const Gaffer::FloatPlug *PlatonicLevelSet::halfWidthPlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 5 );
}

void PlatonicLevelSet::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
    ObjectSource::affects( input, outputs );

    if( input == facesPlug() ||
        input == gridPlug() ||
        input == scalePlug() ||
        input->parent() == centerPlug() ||
        input == voxelSizePlug() ||
        input == halfWidthPlug()
        )
    {
        outputs.push_back( sourcePlug() );
    }
}

void PlatonicLevelSet::hashSource( const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
    facesPlug()->hash( h );
    gridPlug()->hash( h );
    scalePlug()->hash( h );
    h.append( centerPlug()->hash() );
    voxelSizePlug()->hash( h );
    halfWidthPlug()->hash( h );
}

IECore::ConstObjectPtr PlatonicLevelSet::computeSource( const Context *context ) const
{
    Interrupter interrupter( context->canceller() );

    const auto center = centerPlug()->getValue();

    openvdb::FloatGrid::Ptr grid = openvdb::tools::createLevelSetPlatonic<openvdb::FloatGrid, Interrupter>(
            facesPlug()->getValue(),
            scalePlug()->getValue(),
            openvdb::Vec3f(center.x, center.y, center.z),
            voxelSizePlug()->getValue(),
            halfWidthPlug()->getValue(),
            &interrupter);

    if ( interrupter.wasInterrupted() )
    {
        throw IECore::Cancelled();
    }

    grid->setName( gridPlug()->getValue() );

    VDBObjectPtr newVDBObject =  new VDBObject();
    newVDBObject->insertGrid( grid );

    return newVDBObject;

}

