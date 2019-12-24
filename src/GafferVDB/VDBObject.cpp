//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018, Image Engine. All rights reserved.
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
//      * Neither the name of Image Engine nor the names of
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

#include "GafferVDB/VDBObject.h"

#include "Gaffer/StringPlug.h"

#include "IECoreScene/MeshPrimitive.h"

#include "IECoreVDB/VDBObject.h"


using namespace Gaffer;
using namespace GafferScene;
using namespace GafferVDB;
using namespace Imath;
using namespace IECore;
using namespace IECoreScene;

IE_CORE_DEFINERUNTIMETYPED( VDBObject );

size_t VDBObject::g_firstPlugIndex = 0;

VDBObject::VDBObject( const std::string &name )
		:	ObjectSource( name, "vdb" )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new V3iPlug( "dimensions", Plug::In, V3f( 1.0f ), V3f( 0.0f ) ) );
	addChild( new IntPlug( "gridType", Plug::In, 0 ) );
    addChild( new StringPlug( "gridName", Plug::In, "" ) );
}

VDBObject::~VDBObject()
{
}

Gaffer::V3iPlug *VDBObject::dimensionsPlug()
{
	return getChild<V3iPlug>( g_firstPlugIndex );
}

const Gaffer::V3iPlug *VDBObject::dimensionsPlug() const
{
	return getChild<V3iPlug>( g_firstPlugIndex );
}

Gaffer::IntPlug *VDBObject::gridTypePlug()
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::IntPlug *VDBObject::gridTypePlug() const
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

Gaffer::StringPlug *VDBObject::gridNamePlug()
{
    return getChild<StringPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::StringPlug *VDBObject::gridNamePlug() const
{
    return getChild<StringPlug>( g_firstPlugIndex + 2 );
}

void VDBObject::affects( const Plug *input, AffectedPlugsContainer &outputs ) const
{
	ObjectSource::affects( input, outputs );

	if( input->parent<V3iPlug>() == dimensionsPlug() || input == gridTypePlug() || input == gridNamePlug() )
	{
		outputs.push_back( sourcePlug() );
	}
}

void VDBObject::hashSource( const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	dimensionsPlug()->hash( h );
    gridTypePlug()->hash( h );
    gridNamePlug()->hash( h );
}

IECore::ConstObjectPtr VDBObject::computeSource( const Context *context ) const
{
	auto vdbObject = new IECoreVDB::VDBObject();

	//V3i dimensions = dimensionsPlug()->getValue();

	const bool isFog = gridTypePlug()->getValue() == 0;
	const float background = isFog ? 0.0f : 1.0f;

	openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create( background );

	grid->setTransform( openvdb::math::Transform::createLinearTransform(/*voxel size=*/0.5) );
	grid->setName( gridNamePlug()->getValue() );

	if ( isFog )
    {
        grid->setGridClass( openvdb::GRID_FOG_VOLUME );
    }
	else
    {
        grid->setGridClass( openvdb::GRID_LEVEL_SET );
    }

	grid->addStatsMetadata();

	vdbObject->insertGrid( grid );

	return vdbObject;
}

