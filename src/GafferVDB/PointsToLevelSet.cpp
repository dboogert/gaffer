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

#include "GafferVDB/PointsToLevelSet.h"

#include "IECore/StringAlgo.h"
#include "IECore/VectorTypedData.h"
#include "IECoreScene/Primitive.h"

#include "IECoreVDB/VDBObject.h"

#include "Gaffer/StringPlug.h"

#include "openvdb/tools/ParticlesToLevelSet.h"

using namespace std;
using namespace Imath;
using namespace IECore;
using namespace IECoreVDB;
using namespace Gaffer;
using namespace GafferVDB;

IE_CORE_DEFINERUNTIMETYPED( PointsToLevelSet );

size_t PointsToLevelSet::g_firstPlugIndex = 0;


namespace
{
	class ParticleList
	{
	public:

		ParticleList( IECoreScene::ConstPrimitivePtr primitive, float radiusScale, float velocityScale )
		: primitive( primitive ),
          radiusScale(radiusScale),
		  velocityScale(velocityScale)
		{
			auto data =  primitive->variables.find( "P" )->second.data;

			auto vectorData = IECore::runTimeCast<IECore::V3fVectorData>( data );
			positions = &vectorData->readable();

			auto widthIt = primitive->variables.find( "width" );

			if ( widthIt != primitive->variables.end() )
            {
                // todo name `width` parameter
                auto widthData = IECore::runTimeCast<IECore::FloatVectorData>( widthIt->second.data );
                widths = &widthData->readable();
            }
			else
            {
                widths = nullptr;
			}

            auto velocityIt = primitive->variables.find( "velocity" );

            if ( velocityIt != primitive->variables.end() )
            {
                // todo name `velocity` parameter
                auto velocityData = IECore::runTimeCast<IECore::V3fVectorData>( velocityIt->second.data );
                velocities = &velocityData->readable();
            }
            else
            {
                velocities = nullptr;
            }
		}

		typedef openvdb::Vec3R  PosType;

		// Return the total number of particles in list.
		// Always required!
		size_t size() const
		{
			return positions->size();
		}

		// Get the world space position of the nth particle.
		// Required by ParticledToLevelSet::rasterizeSphere(*this,radius).
		void getPos(size_t n, openvdb::Vec3R& xyz) const
		{
			auto p = (*positions)[n];
			xyz =  openvdb::Vec3R(p[0],p[1], p[2] );
		}

		// Get the world space position and radius of the nth particle.
		// Required by ParticledToLevelSet::rasterizeSphere(*this).
		void getPosRad(size_t n, openvdb::Vec3R& xyz, openvdb::Real& rad) const
		{
			auto p = (*positions)[n];
			xyz =  openvdb::Vec3R(p[0],p[1], p[2] );
			rad = widths ? (*widths)[n] : 1.0f;
			rad *= radiusScale;
		}


		// Get the world space position, radius and velocity of the nth particle.
		// Required by ParticledToLevelSet::rasterizeSphere(*this,radius).
		void getPosRadVel(size_t n, openvdb::Vec3R& xyz, openvdb::Real& rad, openvdb::Vec3R& vel) const
		{
			auto p = (*positions)[n];
			auto v = velocities ? (*velocities)[n] : Imath::V3f(0,0,0);
			xyz =  openvdb::Vec3R(p[0],p[1], p[2] );
			rad = widths ? (*widths)[n] : 1.0f;
			rad *= radiusScale;
            vel =  openvdb::Vec3R( v[0], v[1], v[2] );
            vel *= velocityScale;
		}

		// Get the attribute of the nth particle. AttributeType is user-defined!
		// Only required if attribute transfer is enabled in ParticlesToLevelSet.
//		void getAtt(size_t n, AttributeType& att) const
//		{
//
//		}

        bool hasVelocity() const
        {
		    return velocities != nullptr;
        }

		IECoreScene::ConstPrimitivePtr primitive;
		const std::vector<Imath::V3f> *positions;
		const std::vector<float> *widths;
        const std::vector<Imath::V3f> * velocities;
		float radiusScale;
		float velocityScale;
	};
}

PointsToLevelSet::PointsToLevelSet( const std::string &name )
		:	SceneElementProcessor( name, IECore::PathMatcher::NoMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new GafferScene::ScenePlug( "in", Gaffer::Plug::In ) );

	addChild( new StringPlug( "pointsLocation", Plug::In, "") );
	addChild( new StringPlug( "grid", Plug::In, "") );

    addChild( new FloatPlug( "radiusScale", Plug::In, 1.0f, 0.0f) );
    addChild( new BoolPlug( "trails", Plug::In, false) );
    addChild( new FloatPlug( "trailDelta", Plug::In, 0.5f) );
    addChild( new FloatPlug( "velocityScale", Plug::In, 1.0f, 0.0f) );
}

PointsToLevelSet::~PointsToLevelSet()
{
}

GafferScene::ScenePlug *PointsToLevelSet::otherPlug()
{
	return  getChild<GafferScene::ScenePlug>( g_firstPlugIndex );
}

const GafferScene::ScenePlug *PointsToLevelSet::otherPlug() const
{
	return  getChild<GafferScene::ScenePlug>( g_firstPlugIndex );
}

Gaffer::StringPlug *PointsToLevelSet::pointsLocationPlug()
{
	return  getChild<StringPlug>( g_firstPlugIndex + 1);
}

const Gaffer::StringPlug *PointsToLevelSet::pointsLocationPlug() const
{
	return  getChild<StringPlug>( g_firstPlugIndex + 1);
}

Gaffer::StringPlug *PointsToLevelSet::gridPlug()
{
	return  getChild<StringPlug>( g_firstPlugIndex + 2);
}

const Gaffer::StringPlug *PointsToLevelSet::gridPlug() const
{
	return  getChild<StringPlug>( g_firstPlugIndex + 2 );
}

Gaffer::FloatPlug *PointsToLevelSet::radiusScalePlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::FloatPlug *PointsToLevelSet::radiusScalePlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 3 );
}

Gaffer::BoolPlug *PointsToLevelSet::trailsPlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex + 4 );
}

const Gaffer::BoolPlug *PointsToLevelSet::trailsPlug() const
{
    return getChild<BoolPlug>( g_firstPlugIndex + 4 );
}

Gaffer::FloatPlug *PointsToLevelSet::trailDeltaPlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 5 );
}

const Gaffer::FloatPlug *PointsToLevelSet::trailDeltaPlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 5 );
}

Gaffer::FloatPlug *PointsToLevelSet::velocityScalePlug()
{
    return getChild<FloatPlug>( g_firstPlugIndex + 6 );
}

const Gaffer::FloatPlug *PointsToLevelSet::velocityScalePlug() const
{
    return getChild<FloatPlug>( g_firstPlugIndex + 6 );
}


void PointsToLevelSet::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );

	if( input == gridPlug() || input == pointsLocationPlug() || input == radiusScalePlug() || input == velocityScalePlug()
	|| input == trailsPlug() || input == trailDeltaPlug() || input == otherPlug() )
	{
		outputs.push_back( outPlug()->objectPlug() );
		outputs.push_back( outPlug()->boundPlug() );
	}
}

bool PointsToLevelSet::processesObject() const
{
	return true;
}

void PointsToLevelSet::hashProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	SceneElementProcessor::hashProcessedObject( path, context, h );

	gridPlug()->hash( h );

	GafferScene::ScenePlug::ScenePath pointsLocation ;
	GafferScene::ScenePlug::stringToPath( pointsLocationPlug()->getValue(), pointsLocation);

	h.append( otherPlug()->objectHash( pointsLocation ) );
	h.append( otherPlug()->fullTransformHash( pointsLocation ) );
	h.append( radiusScalePlug()->hash() );
	h.append( trailsPlug()->hash() );
	h.append( velocityScalePlug()->hash() );
	h.append( trailDeltaPlug()->hash() );
}

IECore::ConstObjectPtr PointsToLevelSet::computeProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::ConstObjectPtr inputObject ) const
{
	const VDBObject *vdbObject = runTimeCast<const VDBObject>(inputObject.get());
	if( !vdbObject )
	{
		return inputObject;
	}

	VDBObjectPtr newVDBObject = vdbObject->copy();

	std::string gridName = gridPlug()->getValue();

	openvdb::GridBase::Ptr grid = newVDBObject->findGrid( gridName );

	if (!grid)
	{
		return inputObject;
	}

	openvdb::FloatGrid::Ptr floatGrid = openvdb::GridBase::grid<openvdb::FloatGrid>( grid );

	if (!floatGrid)
	{
		return inputObject;
	}

	openvdb::tools::ParticlesToLevelSet<openvdb::FloatGrid> toLevelSet ( *floatGrid );

	GafferScene::ScenePlug::ScenePath pointsLocation ;
	GafferScene::ScenePlug::stringToPath( pointsLocationPlug()->getValue(), pointsLocation );

	IECoreScene::ConstPrimitivePtr pointsPrimitive = runTimeCast<const IECoreScene::Primitive>( otherPlug()->object( pointsLocation ) );

	ParticleList particleList( pointsPrimitive, radiusScalePlug()->getValue(), velocityScalePlug()->getValue() );

	const bool rasteriseTrails = trailsPlug()->getValue() ;

	if ( rasteriseTrails )
    {
	    if ( !particleList.hasVelocity() )
        {
            throw IECore::Exception( "particles require `velocity` V3f attribute to render trails" );
        }
        toLevelSet.rasterizeTrails( particleList, trailDeltaPlug()->getValue() );
    }
	else
    {
        toLevelSet.rasterizeSpheres( particleList );
    }
	return newVDBObject;
}

bool PointsToLevelSet::processesBound() const
{
	return true;
}

void PointsToLevelSet::hashProcessedBound( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	SceneElementProcessor::hashProcessedBound( path, context, h );

	hashProcessedObject( path, context, h);
}

Imath::Box3f PointsToLevelSet::computeProcessedBound( const ScenePath &path, const Gaffer::Context *context, const Imath::Box3f &inputBound ) const
{
	// todo calculate bounds from vdb grids
	return inputBound;
}
