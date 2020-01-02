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

#include "GafferVDB/LevelSetMeasure.h"
#include "GafferVDB/Interrupt.h"
#include "GafferVDB/Dispatcher.h"

#include "IECore/StringAlgo.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECoreVDB/VDBObject.h"

#include "Gaffer/StringPlug.h"
#include "GafferScene/ScenePlug.h"

#include "openvdb/openvdb.h"
#include "openvdb/tools/LevelSetMeasure.h"


using namespace std;
using namespace Imath;
using namespace IECore;
using namespace IECoreVDB;
using namespace Gaffer;
using namespace GafferScene;
using namespace GafferVDB;

namespace
{
    //! results of measuring the level set
    struct Measurements
    {
        Measurements() : area(0.0), volume(0.0), averageMeanCurvature(0.0), valid(false), hasCurvature(false) {}

        openvdb::Real area;
        openvdb::Real volume;
        openvdb::Real averageMeanCurvature;

        bool valid;
        bool hasCurvature;
    };

    //! The kernel of the measurement operation as a template to support different grid types
    template<typename G>
    class Measurer
    {
    public:

        Measurements operator()( typename G::ConstPtr grid, const LevelSetMeasure* node, const Gaffer::Context* context) const
        {
            Interrupter interrupter( context->canceller() );

            openvdb::tools::LevelSetMeasure<G, Interrupter> measure ( *grid, &interrupter );

            Measurements measurements;
            measurements.valid = true;

            const bool calculateCurvature = node->curvaturePlug()->getValue();
            const bool worldUnits = node->worldUnitsPlug()->getValue();

            if ( calculateCurvature )
            {
                measure.measure( measurements.area, measurements.volume, measurements.averageMeanCurvature, worldUnits );
                measurements.hasCurvature = true;
            }
            else
            {
                measure.measure( measurements.area, measurements.volume, worldUnits );
            }

            if ( interrupter.wasInterrupted() )
            {
                throw IECore::Cancelled();
            }

            return measurements;
        }
    };

} // namespace

IE_CORE_DEFINERUNTIMETYPED( LevelSetMeasure );

size_t LevelSetMeasure::g_firstPlugIndex = 0;

LevelSetMeasure::LevelSetMeasure( const std::string &name )
		: SceneElementProcessor( name, IECore::PathMatcher::NoMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild ( new StringPlug( "grids", Plug::In, "*" ) );
    addChild ( new BoolPlug( "curvature", Plug::In, false ) );
    addChild ( new BoolPlug( "worldUnits", Plug::In, false ) );

	outPlug()->boundPlug()->setInput( inPlug()->boundPlug() );
	outPlug()->objectPlug()->setInput( inPlug()->objectPlug() );
}

LevelSetMeasure::~LevelSetMeasure()
{
}

Gaffer::StringPlug *LevelSetMeasure::gridsPlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *LevelSetMeasure::gridsPlug() const
{
	return getChild<const StringPlug>( g_firstPlugIndex );
}

Gaffer::BoolPlug *LevelSetMeasure::curvaturePlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::BoolPlug *LevelSetMeasure::curvaturePlug() const
{
    return getChild<const BoolPlug>( g_firstPlugIndex + 1 );
}

Gaffer::BoolPlug *LevelSetMeasure::worldUnitsPlug()
{
    return getChild<BoolPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::BoolPlug *LevelSetMeasure::worldUnitsPlug() const
{
    return getChild<const BoolPlug>( g_firstPlugIndex + 2 );
}

void LevelSetMeasure::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );

	if ( input == gridsPlug() || input == curvaturePlug() || input == worldUnitsPlug() )
	{
		outputs.push_back( outPlug()->attributesPlug() );
	}
}

bool LevelSetMeasure::processesAttributes() const
{
	return true;
}

void LevelSetMeasure::hashProcessedAttributes( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	SceneElementProcessor::hashProcessedAttributes( path, context, h );

	h.append( inPlug()->objectHash( path ) );
	h.append( gridsPlug()->hash() );
	h.append( curvaturePlug()->hash() );
	h.append( worldUnitsPlug()->hash() );
}

IECore::ConstCompoundObjectPtr LevelSetMeasure::computeProcessedAttributes( const ScenePath &path, const Gaffer::Context *context, IECore::ConstCompoundObjectPtr inputAttributes ) const
{
	auto vdbObject = IECore::runTimeCast<const IECoreVDB::VDBObject> ( inPlug()->object( path ) );

	if ( !vdbObject )
	{
		return inputAttributes;
	}

	std::vector<std::string> gridNames = vdbObject->gridNames();
	std::string grids = gridsPlug()->getValue();

	IECore::CompoundObjectPtr newAttributes = inputAttributes->copy();

    ScalarGridDispatcher<Measurer, LevelSetMeasure, Measurements> measurer( this, context );

	for ( const auto &gridName : gridNames )
	{
		if ( IECore::StringAlgo::matchMultiple( gridName, grids ) )
		{
			openvdb::GridBase::ConstPtr srcGrid = vdbObject->findGrid( gridName );

            Measurements measurements = measurer( srcGrid );

            if ( measurements.valid )
            {
                newAttributes->members().insert(std::make_pair( std::string("levelset:") + gridName + ":area", new IECore::DoubleData( measurements.area ) ) );
                newAttributes->members().insert(std::make_pair( std::string("levelset:") + gridName + ":volume", new IECore::DoubleData( measurements.volume ) ) );

                if ( measurements.hasCurvature )
                {
                    newAttributes->members().insert(std::make_pair( std::string("levelset:") + gridName + ":averageMeanCurvature", new IECore::DoubleData( measurements.averageMeanCurvature ) ) );
                }
            }
		}
	}

	return newAttributes;
}


