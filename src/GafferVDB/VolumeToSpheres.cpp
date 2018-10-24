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

#include "GafferVDB/VolumeToSpheres.h"

#include "IECore/StringAlgo.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECoreVDB/VDBObject.h"

#include "Gaffer/StringPlug.h"

#include "openvdb/openvdb.h"
#include "openvdb/tools/VolumeToSpheres.h"

using namespace std;
using namespace Imath;
using namespace IECore;
using namespace IECoreVDB;
using namespace Gaffer;
using namespace GafferVDB;

IE_CORE_DEFINERUNTIMETYPED( VolumeToSpheres );

size_t VolumeToSpheres::g_firstPlugIndex = 0;

VolumeToSpheres::VolumeToSpheres( const std::string &name )
		: SceneElementProcessor( name, IECore::PathMatcher::NoMatch )
{
	storeIndexOfNextChild(g_firstPlugIndex);

	addChild( new StringPlug( "grids", Plug::In, "surface" ) );
    addChild( new IntPlug( "maxSpheres", Plug::In, 100, 0 ) );
    addChild( new BoolPlug( "overlapping", Plug::In, false ) );
    addChild( new FloatPlug( "minRadius", Plug::In, 0.0f, 0.0f ) );
    addChild( new FloatPlug( "maxRadius", Plug::In, 10.0f, 0.0f ) );
    addChild( new FloatPlug( "isoValue", Plug::In, 0.0f, 0.0f ) );
}

VolumeToSpheres::~VolumeToSpheres()
{
}

Gaffer::StringPlug *VolumeToSpheres::gridsPlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *VolumeToSpheres::gridsPlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

Gaffer::IntPlug *VolumeToSpheres::maxSpheresPlug()
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::IntPlug *VolumeToSpheres::maxSpheresPlug() const
{
    return getChild<IntPlug>( g_firstPlugIndex + 1 );
}

Gaffer::BoolPlug *VolumeToSpheres::overlappingPlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::BoolPlug *VolumeToSpheres::overlappingPlug() const
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

Gaffer::FloatPlug *VolumeToSpheres::minRadiusPlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::FloatPlug *VolumeToSpheres::minRadiusPlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

Gaffer::FloatPlug *VolumeToSpheres::maxRadiusPlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 4 );
}

const Gaffer::FloatPlug *VolumeToSpheres::maxRadiusPlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 4 );
}

Gaffer::FloatPlug *VolumeToSpheres::isoValuePlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 5 );
}

const Gaffer::FloatPlug *VolumeToSpheres::isoValuePlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 5 );
}

void VolumeToSpheres::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );


	if( input == gridsPlug() || input == maxSpheresPlug() || input == overlappingPlug() || input == minRadiusPlug() || input == maxRadiusPlug() || input == isoValuePlug() )
	{
		outputs.push_back( outPlug()->objectPlug() );
		outputs.push_back( outPlug()->boundPlug() );
	}
}

bool VolumeToSpheres::processesObject() const
{
	return true;
}

void VolumeToSpheres::hashProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	SceneElementProcessor::hashProcessedObject( path, context, h );

	h.append( gridsPlug()->hash() );
	h.append( maxSpheresPlug()->hash() );
	h.append( overlappingPlug()->hash() );
	h.append( minRadiusPlug()->hash() );
	h.append( maxRadiusPlug()->hash() );
	h.append( isoValuePlug()->hash() );
}

IECore::ConstObjectPtr VolumeToSpheres::computeProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::ConstObjectPtr inputObject ) const
{
	const IECoreVDB::VDBObject *vdbObject = runTimeCast<const IECoreVDB::VDBObject>(inputObject.get());
	if( !vdbObject )
	{
		return inputObject;
	}

	std::vector<std::string> grids = vdbObject->gridNames();

	std::string gridsToProcess = gridsPlug()->getValue();

	IECoreVDB::VDBObjectPtr newVDBObject = vdbObject->copy();

	for (const auto &gridName : grids )
	{
		if (IECore::StringAlgo::matchMultiple(gridName, gridsToProcess))
		{
			openvdb::GridBase::ConstPtr grid = vdbObject->findGrid( gridName );
			openvdb::FloatGrid::ConstPtr floatGrid = openvdb::GridBase::constGrid<openvdb::FloatGrid>( grid );

			if ( floatGrid )
			{

				std::vector<openvdb::Vec4f> spheres;
				openvdb::tools::fillWithSpheres<openvdb::FloatGrid>( *floatGrid, spheres, maxSpheresPlug()->getValue(), overlappingPlug()->getValue(), minRadiusPlug()->getValue(), maxRadiusPlug()->getValue(), isoValuePlug()->getValue() );

				IECore::V3fVectorDataPtr positions = new IECore::V3fVectorData();
				IECore::FloatVectorDataPtr radii = new IECore::FloatVectorData();

				auto &wposArray = positions->writable();
				auto &sizeArray = radii->writable();

				for (const auto s : spheres)
				{
					wposArray.push_back(Imath::V3f(s[0], s[1], s[2]));
					sizeArray.push_back(s[3] * 2.0f);
				}


				IECoreScene::PointsPrimitivePtr pointsPrimitive = new IECoreScene::PointsPrimitive( positions );
				pointsPrimitive->variables["width"] = IECoreScene::PrimitiveVariable(IECoreScene::PrimitiveVariable::Interpolation::Vertex,
						radii);

				return pointsPrimitive;

			}
		}
	}

	return newVDBObject;
}

bool VolumeToSpheres::processesBound() const
{
	return true;
}

void VolumeToSpheres::hashProcessedBound( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	SceneElementProcessor::hashProcessedBound( path, context, h );

	hashProcessedObject( path, context, h);
}

Imath::Box3f VolumeToSpheres::computeProcessedBound( const ScenePath &path, const Gaffer::Context *context, const Imath::Box3f &inputBound ) const
{
	// todo calculate bounds from vdb grids
	return inputBound;
}

