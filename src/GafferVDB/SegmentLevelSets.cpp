//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2020, Don Boogert. All rights reserved.
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

#include "GafferVDB/SegmentLevelSets.h"

#include "IECore/StringAlgo.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECoreVDB/VDBObject.h"

#include "Gaffer/StringPlug.h"
#include "GafferScene/ScenePlug.h"

#include "openvdb/openvdb.h"
#include "openvdb/tools/LevelSetUtil.h"


using namespace std;
using namespace Imath;
using namespace IECore;
using namespace IECoreVDB;
using namespace Gaffer;
using namespace GafferScene;
using namespace GafferVDB;

IE_CORE_DEFINERUNTIMETYPED(SegmentLevelSets );

size_t SegmentLevelSets::g_firstPlugIndex = 0;

SegmentLevelSets::SegmentLevelSets(const std::string &name )
		: SceneElementProcessor( name, IECore::PathMatcher::NoMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild ( new StringPlug( "grids", Plug::In, "*" ) );
    addChild ( new StringPlug( "outputGrid", Plug::In, "${grid}_${segment}" ) );
}

SegmentLevelSets::~SegmentLevelSets()
{
}

Gaffer::StringPlug *SegmentLevelSets::gridsPlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *SegmentLevelSets::gridsPlug() const
{
	return getChild<const StringPlug>( g_firstPlugIndex );
}

Gaffer::StringPlug *SegmentLevelSets::outputGridPlug()
{
    return  getChild<StringPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::StringPlug *SegmentLevelSets::outputGridPlug() const
{
    return  getChild<StringPlug>( g_firstPlugIndex + 1 );
}

void SegmentLevelSets::affects(const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );

	if ( input == gridsPlug() || input == outputGridPlug() )
	{
		outputs.push_back( outPlug()->objectPlug() );
	}
}

bool SegmentLevelSets::processesObject() const
{
    return true;
}

void SegmentLevelSets::hashProcessedObject(const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
    SceneElementProcessor::hashProcessedObject( path, context, h );

    h.append( inPlug()->objectHash( path ) );
    h.append( gridsPlug()->getValue() );
    h.append( outputGridPlug()->getValue() );
}

IECore::ConstObjectPtr SegmentLevelSets::computeProcessedObject(const ScenePath &path, const Gaffer::Context *context, IECore::ConstObjectPtr inputObject ) const
{
	auto vdbObject = IECore::runTimeCast<const IECoreVDB::VDBObject> ( inPlug()->object( path ) );

	if ( !vdbObject )
	{
		return inputObject;
	}

	std::vector<std::string> gridNames = vdbObject->gridNames();
	std::string grids = gridsPlug()->getValue();

    VDBObjectPtr newVDBObject = vdbObject->copy();

	for (const auto &gridName : gridNames )
	{
		if (!IECore::StringAlgo::matchMultiple(gridName, grids))
		{
            continue;
        }

        openvdb::GridBase::ConstPtr srcGrid = vdbObject->findGrid( gridName );

        openvdb::FloatGrid::ConstPtr grid = openvdb::GridBase::constGrid<openvdb::FloatGrid>( srcGrid );

        if ( !grid )
        {
            continue;
        }

        Context::EditableScope scope( context );
        scope.set( IECore::InternedString("grid"), gridName );

        std::vector<openvdb::FloatGrid::Ptr> segments;
        openvdb::tools::segmentSDF( *grid, segments );

        int i = 0;
        for ( auto segment : segments )
        {
            scope.set( IECore::InternedString("segment"), i );
            const std::string outGridName = context->substitute( outputGridPlug()->getValue() );
            segment->setName( outGridName );
            newVDBObject->insertGrid( segment );
            i++;
        }
	}

	return newVDBObject;
}


