//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2017, Image Engine. All rights reserved.
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

#include "GafferVDB/OffsetLevelSet.h"

#include "IECoreVDB/VDBObject.h"

#include "Gaffer/StringPlug.h"

#include "openvdb/openvdb.h"
#include "openvdb/tools/LevelSetFilter.h"

using namespace std;
using namespace Imath;
using namespace IECore;
using namespace IECoreVDB;
using namespace Gaffer;
using namespace GafferVDB;

GAFFER_GRAPHCOMPONENT_DEFINE_TYPE(OffsetLevelSet );

size_t OffsetLevelSet::g_firstPlugIndex = 0;

OffsetLevelSet::OffsetLevelSet(const std::string &name )
	:	SceneElementProcessor( name, IECore::PathMatcher::NoMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );

	addChild( new StringPlug( "grids", Plug::In, "*") );
    addChild( new StringPlug( "outputGrid", Plug::In, "${grid}" ) );
	addChild( new FloatPlug( "offset", Plug::In, 0.5) );
}

OffsetLevelSet::~OffsetLevelSet()
{
}

Gaffer::StringPlug *OffsetLevelSet::gridsPlug()
{
	return  getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *OffsetLevelSet::gridsPlug() const
{
	return  getChild<StringPlug>( g_firstPlugIndex );
}

Gaffer::StringPlug *OffsetLevelSet::outputGridPlug()
{
    return  getChild<StringPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::StringPlug *OffsetLevelSet::outputGridPlug() const
{
    return  getChild<StringPlug>( g_firstPlugIndex + 1 );
}

Gaffer::FloatPlug *OffsetLevelSet::offsetPlug()
{
	return  getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::FloatPlug *OffsetLevelSet::offsetPlug() const
{
	return  getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

void OffsetLevelSet::affects(const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );

	if(input == gridsPlug() || input == offsetPlug() || input == outputGridPlug() )
	{
		outputs.push_back( outPlug()->objectPlug() );
		outputs.push_back( outPlug()->boundPlug() );
	}
}

bool OffsetLevelSet::processesObject() const
{
	return true;
}

void OffsetLevelSet::hashProcessedObject(const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	SceneElementProcessor::hashProcessedObject( path, context, h );

    gridsPlug()->hash( h );
    outputGridPlug()->hash( h );
	offsetPlug()->hash( h );
}

IECore::ConstObjectPtr OffsetLevelSet::computeProcessedObject(const ScenePath &path, const Gaffer::Context *context, IECore::ConstObjectPtr inputObject ) const
{
	const VDBObject *vdbObject = runTimeCast<const VDBObject>(inputObject.get());
	if( !vdbObject )
	{
		return inputObject;
	}

    std::vector<std::string> grids = vdbObject->gridNames();

    std::string gridsToProcess = gridsPlug()->getValue();

    VDBObjectPtr newVDBObject = vdbObject->copy();

    for (const auto &gridName : grids )
    {
        Context::EditableScope scope(context);
        scope.set(IECore::InternedString("grid"), gridName);
        const std::string outGridName = context->substitute(outputGridPlug()->getValue());

        openvdb::GridBase::ConstPtr gridBase = vdbObject->findGrid( gridName );

        if ( !gridBase )
        {
           continue;
        }

        openvdb::GridBase::Ptr newGrid;

        if ( openvdb::FloatGrid::ConstPtr floatGrid = openvdb::GridBase::constGrid<openvdb::FloatGrid>( gridBase ) )
        {
            openvdb::FloatGrid::Ptr newFloatGrid = openvdb::GridBase::grid<openvdb::FloatGrid> ( floatGrid->deepCopyGrid() );
            newGrid = newFloatGrid;
            openvdb::tools::LevelSetFilter <openvdb::FloatGrid> filter( *newFloatGrid );
            filter.offset( offsetPlug()->getValue() );
        }
        else if ( openvdb::DoubleGrid::ConstPtr doubleGrid = openvdb::GridBase::constGrid<openvdb::DoubleGrid>( newGrid ) )
        {
            openvdb::DoubleGrid::Ptr newDoubleGrid = openvdb::GridBase::grid<openvdb::DoubleGrid>( doubleGrid->deepCopyGrid() );
            newGrid = newDoubleGrid;
            openvdb::tools::LevelSetFilter <openvdb::DoubleGrid> filter( *newDoubleGrid );
            filter.offset( offsetPlug()->getValue() );
        }

        if ( newGrid )
        {
            newGrid->setName( outGridName );
            newVDBObject->insertGrid( newGrid );
        }

    }

	return newVDBObject;
}


bool OffsetLevelSet::processesBound() const
{
	return true;
}

void OffsetLevelSet::hashProcessedBound(const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	SceneElementProcessor::hashProcessedBound( path, context, h );

    gridsPlug()->hash(h );
	offsetPlug()->hash( h );
}

Imath::Box3f OffsetLevelSet::computeProcessedBound(const ScenePath &path, const Gaffer::Context *context, const Imath::Box3f &inputBound ) const
{
	Imath::Box3f newBound = inputBound;
	float offset = -offsetPlug()->getValue();

	newBound.min -= Imath::V3f(offset, offset, offset);
	newBound.max += Imath::V3f(offset, offset, offset);

	return newBound;
}
