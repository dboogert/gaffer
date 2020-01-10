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

#include "GafferVDB/MaskGrids.h"

#include "IECore/StringAlgo.h"

#include "IECoreScene/PointsPrimitive.h"

#include "IECoreVDB/VDBObject.h"

#include "Gaffer/StringPlug.h"

#include "openvdb/openvdb.h"
#include "openvdb/tools/Mask.h"

using namespace std;
using namespace Imath;
using namespace IECore;
using namespace IECoreVDB;
using namespace Gaffer;
using namespace GafferVDB;
using namespace GafferScene;

IE_CORE_DEFINERUNTIMETYPED( MaskGrids );

size_t MaskGrids::g_firstPlugIndex = 0;

MaskGrids::MaskGrids(const std::string &name )
        : SceneElementProcessor( name, IECore::PathMatcher::NoMatch )
{
    storeIndexOfNextChild(g_firstPlugIndex);

    addChild( new StringPlug( "grids", Plug::In, "*" ) );
    addChild( new StringPlug( "outputGrid", Plug::In, "${grid}" ) );
    addChild( new FloatPlug( "iso", Plug::In, 0.0f ) );

}

MaskGrids::~MaskGrids()
{
}

Gaffer::StringPlug *MaskGrids::gridsPlug()
{
    return  getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *MaskGrids::gridsPlug() const
{
    return  getChild<StringPlug>( g_firstPlugIndex );
}

Gaffer::StringPlug *MaskGrids::outputGridPlug()
{
    return  getChild<StringPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::StringPlug *MaskGrids::outputGridPlug() const
{
    return  getChild<StringPlug>( g_firstPlugIndex + 1 );
}

Gaffer::FloatPlug *MaskGrids::isoPlug()
{
    return  getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::FloatPlug *MaskGrids::isoPlug() const
{
    return  getChild<FloatPlug>( g_firstPlugIndex + 2 );
}

void MaskGrids::affects(const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
    SceneElementProcessor::affects( input, outputs );

    if( input == gridsPlug() ||
        input == outputGridPlug() ||
        input == isoPlug())
    {
        outputs.push_back( outPlug()->objectPlug() );
        outputs.push_back( outPlug()->boundPlug() );
    }
}

bool MaskGrids::processesObject() const
{
    return true;
}

void MaskGrids::hashProcessedObject(const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
    SceneElementProcessor::hashProcessedObject( path, context, h );

    gridsPlug()->hash( h );
    isoPlug()->hash( h );
    outputGridPlug()->hash( h );
}

IECore::ConstObjectPtr MaskGrids::computeProcessedObject(const ScenePath &path, const Gaffer::Context *context, IECore::ConstObjectPtr inputObject ) const
{
    const IECoreVDB::VDBObject *vdbObject = runTimeCast<const IECoreVDB::VDBObject>(inputObject.get());
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

        if (!IECore::StringAlgo::matchMultiple(gridName, gridsToProcess))
        {
            continue;
        }

        openvdb::GridBase::ConstPtr grid = vdbObject->findGrid( gridName );
        openvdb::FloatGrid::ConstPtr floatGrid = openvdb::GridBase::constGrid<openvdb::FloatGrid>( grid );

        if ( !floatGrid )
        {
            continue;
        }

        openvdb::FloatGrid::ValueConverter<bool>::Type::Ptr maskGrid = openvdb::tools::interiorMask(*floatGrid, isoPlug()->getValue() );

        maskGrid->setName( outGridName );
        newVDBObject->insertGrid( maskGrid );
    }

    return newVDBObject;
}

bool MaskGrids::processesBound() const
{
    return true;
}

void MaskGrids::hashProcessedBound(const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
    SceneElementProcessor::hashProcessedBound( path, context, h );

    gridsPlug()->hash( h );
}

Imath::Box3f MaskGrids::computeProcessedBound(const ScenePath &path, const Gaffer::Context *context, const Imath::Box3f &inputBound ) const
{
    // todo calculate bounds from vdb grids
    return inputBound;
}


