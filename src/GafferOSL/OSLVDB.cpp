//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2013, Don Boogert. All rights reserved.
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

#include "GafferOSL/OSLVDB.h"

#include "GafferOSL/ClosurePlug.h"
#include "GafferOSL/OSLShader.h"
#include "GafferOSL/ShadingEngine.h"

#include "Gaffer/NameValuePlug.h"
#include "Gaffer/ScriptNode.h"
#include "Gaffer/UndoScope.h"

#include "IECoreVDB/VDBObject.h"

#include "IECore/MessageHandler.h"

#include "ImathVec.h"

#include "openvdb/tools/Interpolation.h"

#include "boost/bind.hpp"


using namespace Imath;
using namespace IECore;
using namespace IECoreScene;
using namespace Gaffer;
using namespace GafferScene;
using namespace GafferOSL;

IE_CORE_DEFINERUNTIMETYPED( OSLVDB );

size_t OSLVDB::g_firstPlugIndex;

enum class CellIterationMode
{
	AllCells,
	ActiveCells,
	InactiveCells
};

namespace
{
	template<typename DestType, typename SrcType>
	void convert( DestType& dst, const SrcType& src )
	{
		dst = src;
	}

	// Cortex -> VDB
	// =============

	template<>
	void convert( openvdb::Vec3f& dst, const Imath::V3f& src )
	{
		dst = openvdb::Vec3f( src.x, src.y, src.z );
	}

	template<>
	void convert( openvdb::Vec3d& dst, const Imath::V3d& src )
	{
		dst = openvdb::Vec3d( src.x, src.y, src.z );
	}

	template<>
	void convert( openvdb::Vec3i& dst, const Imath::V3i& src )
	{
		dst = openvdb::Vec3i( src.x, src.y, src.z );
	}

	// VDB -> Cortex
	// =============

	template<>
	void convert( Imath::V3f &dst, const openvdb::Vec3f& src )
	{
		dst = Imath::V3f( src.x(), src.y(), src.z() );
	}

	template<>
	void convert( Imath::V3d &dst, const openvdb::Vec3d & src )
	{
		dst = Imath::V3d( src.x(), src.y(), src.z() );
	}

	template<>
	void convert( Imath::V3i& dst, const openvdb::Vec3i & src )
	{
		dst = Imath::V3i( src.x(), src.y(), src.z() );
	}

	template<typename GridType, typename VectorDataType>
	openvdb::GridBase::Ptr convert( IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode )
	{
		typename VectorDataType::ConstPtr typedData = runTimeCast<const VectorDataType>( data );
		const auto &readable = typedData->readable();
		int i = 0;

		if ( typename GridType::ConstPtr typedGrid = openvdb::GridBase::constGrid<GridType>( grid ) )
		{
			typename GridType::Ptr newGrid( new GridType( *typedGrid ) );

			if ( iterationMode == CellIterationMode::AllCells )
			{
				for( auto iter = newGrid->tree().beginValueAll(); iter; ++iter )
				{
					typename GridType::ValueType dstValue;
					convert(dstValue, readable[i++]);
					iter.setValue( dstValue );
				}
			}
			else if ( iterationMode == CellIterationMode::ActiveCells )
			{
				for( auto iter = newGrid->tree().beginValueOn(); iter; ++iter )
				{
					typename GridType::ValueType dstValue;
					convert(dstValue, readable[i++]);
					iter.setValue( dstValue );
				}
			}
			else if ( iterationMode == CellIterationMode::InactiveCells )
			{
				for( auto iter = newGrid->tree().beginValueOff(); iter; ++iter )
				{
					typename GridType::ValueType dstValue;
					convert(dstValue, readable[i++]);
					iter.setValue( dstValue );
				}
			}

			return newGrid;
		}

		return nullptr;
	}

	auto convertFrom = []( IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr
	{
		static std::map<std::string, std::function<openvdb::GridBase::Ptr ( IECore::ConstDataPtr, openvdb::GridBase::ConstPtr, CellIterationMode ) > > dataToGridConverters =
		{
			{ "BoolVectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr { return convert<openvdb::BoolGrid, IECore::BoolVectorData>( data, grid, iterationMode ); } },
			{ "DoubleVectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr { return convert<openvdb::DoubleGrid, IECore::DoubleVectorData>( data, grid, iterationMode ); } },
			{ "FloatVectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr{ return convert<openvdb::FloatGrid, IECore::FloatVectorData>( data, grid, iterationMode ); } },
			{ "IntVectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr { return convert<openvdb::Int32Grid, IECore::IntVectorData>( data, grid, iterationMode ); } },
			{ "Int64VectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr { return convert<openvdb::Int64Grid, IECore::Int64VectorData>( data, grid, iterationMode ); } },
			{ "V3fVectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr { return convert<openvdb::Vec3SGrid , IECore::V3fVectorData>( data, grid, iterationMode ); } },
			{ "V3dVectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr { return convert<openvdb::Vec3DGrid , IECore::V3dVectorData>( data, grid, iterationMode ); } },
			{ "V3iVectorData", [](IECore::ConstDataPtr data, openvdb::GridBase::ConstPtr grid, CellIterationMode iterationMode ) -> openvdb::GridBase::Ptr { return convert<openvdb::Vec3IGrid , IECore::V3iVectorData>( data, grid, iterationMode ); } }
		};

		auto it = dataToGridConverters.find( data->typeName() );
		if ( it != dataToGridConverters.end() )
		{
			return it->second( data, grid, iterationMode );
		}

		return nullptr;
	};

	template<typename GridType, typename VectorDataType>
	IECore::DataPtr convert( openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions )
	{
		auto data = new VectorDataType();
		auto &writable = data->writable();

		typename GridType::ConstPtr typedGrid = openvdb::GridBase::constGrid<GridType>( grid );

		openvdb::tools::GridSampler<GridType, openvdb::tools::BoxSampler> sampler( *typedGrid );

		for( const auto &p : positions->readable() )
		{
			typename VectorDataType::ValueType::value_type dst;
			openvdb::Vec3f vdbP;
			convert( vdbP, p);
			convert(dst, sampler.wsSample( vdbP ));
			writable.push_back( dst );
		}

		return data;
	}

	auto convertTo = []( openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions ) -> IECore::DataPtr
	{
		static std::map<std::string, std::function< IECore::DataPtr( openvdb::GridBase::ConstPtr, V3fVectorData::ConstPtr )> > gridToDataConverters =
		{
			{ openvdb::typeNameAsString<bool>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::BoolGrid, IECore::BoolVectorData>(grid, positions); } },
			{ openvdb::typeNameAsString<double>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::DoubleGrid, IECore::DoubleVectorData>(grid, positions); } },
			{ openvdb::typeNameAsString<float>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::FloatGrid, IECore::FloatVectorData>(grid, positions); } },
			{ openvdb::typeNameAsString<int32_t>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::Int32Grid, IECore::IntVectorData>(grid, positions); } },
			{ openvdb::typeNameAsString<int64_t>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::Int64Grid,  IECore::Int64VectorData>(grid, positions); } },
			{ openvdb::typeNameAsString<openvdb::Vec3s>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::Vec3SGrid, IECore::V3fVectorData>(grid, positions); } },
			{ openvdb::typeNameAsString<openvdb::Vec3d>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::Vec3DGrid, IECore::V3dVectorData>(grid, positions); } },
			{ openvdb::typeNameAsString<openvdb::Vec3i>(), [](openvdb::GridBase::ConstPtr grid, V3fVectorData::ConstPtr positions) -> IECore::DataPtr { return convert<openvdb::Vec3IGrid, IECore::V3iVectorData>(grid, positions); } }
		};

		auto it = gridToDataConverters.find( grid->valueType() );
		if ( it != gridToDataConverters.end() )
		{
			return it->second( grid, positions );
		}

		return nullptr;
	};

	template<typename GridType>
	IECore::V3fVectorDataPtr convertPositions( openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode )
	{
		auto data = new V3fVectorData();
		auto &writable = data->writable();

		// todo doesn't have to be a float grid
		if ( typename GridType::ConstPtr grid = openvdb::GridBase::constGrid<GridType>( iterationGrid ) )
		{
			if ( iterationMode == CellIterationMode::AllCells)
			{
				for( auto iter = grid->tree().cbeginValueAll(); iter; ++iter )
				{
					auto worldPosition = iterationGrid->indexToWorld( iter.getCoord() );
					writable.push_back( V3f(worldPosition.x(), worldPosition.y(), worldPosition.z()) );
				}
			}
			else if ( iterationMode == CellIterationMode::ActiveCells )
			{
				for( auto iter = grid->tree().cbeginValueOn(); iter; ++iter )
				{
					auto worldPosition = iterationGrid->indexToWorld( iter.getCoord() );
					writable.push_back( V3f(worldPosition.x(), worldPosition.y(), worldPosition.z()) );
				}
			}
			else if ( iterationMode == CellIterationMode::InactiveCells )
			{
				for( auto iter = grid->tree().cbeginValueOff(); iter; ++iter )
				{
					auto worldPosition = iterationGrid->indexToWorld( iter.getCoord() );
					writable.push_back( V3f(worldPosition.x(), worldPosition.y(), worldPosition.z()) );
				}
			}

			return data;
		}

		return nullptr;
	}

	auto convertToPositions = []( openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode ) -> IECore::V3fVectorDataPtr
	{
		static std::map<std::string, std::function< IECore::V3fVectorDataPtr( openvdb::GridBase::ConstPtr, CellIterationMode )> > positionConverters =
		{
			{ openvdb::typeNameAsString<bool>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::BoolGrid>( iterationGrid, iterationMode); } },
			{ openvdb::typeNameAsString<double>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::DoubleGrid>( iterationGrid, iterationMode); } },
			{ openvdb::typeNameAsString<float>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::FloatGrid>( iterationGrid, iterationMode); } },
			{ openvdb::typeNameAsString<int32_t>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::Int32Grid>( iterationGrid, iterationMode); } },
			{ openvdb::typeNameAsString<int64_t>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::Int64Grid>( iterationGrid, iterationMode); } },
			{ openvdb::typeNameAsString<openvdb::Vec3s>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::Vec3SGrid>( iterationGrid, iterationMode); } },
			{ openvdb::typeNameAsString<openvdb::Vec3d>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::Vec3DGrid>( iterationGrid, iterationMode); } },
			{ openvdb::typeNameAsString<openvdb::Vec3i>(), [](openvdb::GridBase::ConstPtr iterationGrid, CellIterationMode iterationMode) -> IECore::V3fVectorDataPtr { return convertPositions<openvdb::Vec3IGrid>( iterationGrid, iterationMode); } }
		};

		auto it = positionConverters.find( iterationGrid->valueType() );
		if ( it != positionConverters.end() )
		{
			return it->second( iterationGrid, iterationMode );
		}

		return nullptr;
	};


	CompoundDataPtr prepareShadingPoints( const IECoreVDB::VDBObject *vdbObject, openvdb::GridBase::ConstPtr iterationGrid, const ShadingEngine *shadingEngine, CellIterationMode iterationMode )
	{
		CompoundDataPtr shadingPoints = new CompoundData;

		const std::vector<std::string> gridNames = vdbObject->gridNames();

		auto positions =  convertToPositions( iterationGrid, iterationMode );
		shadingPoints->writable()["P"] = positions;

		for( const std::string &gridName : gridNames )
		{
			if( shadingEngine->needsAttribute( gridName ) )
			{
				if ( auto newArray = convertTo( vdbObject->findGrid( gridName ), positions ) )
				{
					shadingPoints->writable()[gridName] = newArray;
				}
			}
		}

		return shadingPoints;
	}

};

OSLVDB::OSLVDB( const std::string &name )
: SceneElementProcessor( name, IECore::PathMatcher::NoMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new ShaderPlug( "__shader",  Plug::In, Plug::Default & ~Plug::Serialisable ) );
    addChild( new OSLCode( "__oslCode" ) );
	addChild( new StringPlug( "iterationGrid", Plug::In, "surface") );
	addChild( new IntPlug( "mode", Plug::In, 0 ) );
    addChild( new Plug( "outputGrids", Plug::In, Plug::Default & ~Plug::AcceptsInputs ) );

    shaderPlug()->setInput( oslCode()->outPlug() );
    outputGridsPlug()->childAddedSignal().connect( boost::bind( &OSLVDB::outputGridAdded, this, ::_1, ::_2 ) );
    outputGridsPlug()->childRemovedSignal().connect( boost::bind( &OSLVDB::outputGridRemoved, this, ::_1, ::_2 ) );

	outPlug()->attributesPlug()->setInput( inPlug()->attributesPlug() );
	outPlug()->transformPlug()->setInput( inPlug()->transformPlug() );
	outPlug()->boundPlug()->setInput( inPlug()->boundPlug() );
}

OSLVDB::~OSLVDB()
{
}

GafferScene::ShaderPlug *OSLVDB::shaderPlug()
{
	return getChild<ShaderPlug>( g_firstPlugIndex );
}

const GafferScene::ShaderPlug *OSLVDB::shaderPlug() const
{
	return getChild<ShaderPlug>( g_firstPlugIndex );
}

GafferOSL::OSLCode *OSLVDB::oslCode()
{
    return getChild<GafferOSL::OSLCode>( g_firstPlugIndex + 1 );
}

const GafferOSL::OSLCode *OSLVDB::oslCode() const
{
    return getChild<GafferOSL::OSLCode>( g_firstPlugIndex + 1 );
}

Gaffer::StringPlug *OSLVDB::iterationGridPlug()
{
	return getChild<StringPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::StringPlug *OSLVDB::iterationGridPlug() const
{
	return getChild<const StringPlug>( g_firstPlugIndex + 2 );
}

Gaffer::IntPlug *OSLVDB::modePlug()
{
	return getChild<IntPlug>( g_firstPlugIndex + 3 );
}

const Gaffer::IntPlug *OSLVDB::modePlug() const
{
	return getChild<const IntPlug>( g_firstPlugIndex + 3 );
}

Gaffer::Plug *OSLVDB::outputGridsPlug()
{
    return getChild<Gaffer::Plug>(g_firstPlugIndex + 4);
}

const Gaffer::Plug *OSLVDB::outputGridsPlug() const
{
    return getChild<Gaffer::Plug>( g_firstPlugIndex + 4 );
}

void OSLVDB::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );

	if( input == shaderPlug() || input == iterationGridPlug() || input == modePlug() )
	{
		outputs.push_back( outPlug()->objectPlug() );
	}
}

bool OSLVDB::acceptsInput( const Gaffer::Plug *plug, const Gaffer::Plug *inputPlug ) const
{
	if( !SceneElementProcessor::acceptsInput( plug, inputPlug ) )
	{
		return false;
	}

	if( !inputPlug )
	{
		return true;
	}

	if( plug == shaderPlug() )
	{
		if( const GafferScene::Shader *shader = runTimeCast<const GafferScene::Shader>( inputPlug->source()->node() ) )
		{
			const OSLShader *oslShader = runTimeCast<const OSLShader>( shader );
			return oslShader && oslShader->typePlug()->getValue() == "osl:surface";
		}
	}

	return true;
}

bool OSLVDB::processesBound() const
{
	return runTimeCast<const OSLShader>( shaderPlug()->source()->node() );
}

void OSLVDB::hashProcessedBound( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	hashProcessedObject( path, context, h );
}

Imath::Box3f OSLVDB::computeProcessedBound( const ScenePath &path, const Gaffer::Context *context, const Imath::Box3f &inputBound ) const
{
	ConstObjectPtr object = outPlug()->objectPlug()->getValue();
	if( const IECoreVDB::VDBObject *vdbObject = runTimeCast<const IECoreVDB::VDBObject>( object.get() ) )
	{
		return vdbObject->bound();
	}
	return inputBound;
}

bool OSLVDB::processesObject() const
{
	return runTimeCast<const OSLShader>( shaderPlug()->source()->node() );
}

void OSLVDB::hashProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	const OSLShader *shader = runTimeCast<const OSLShader>( shaderPlug()->source()->node() );
	ConstShadingEnginePtr shadingEngine = shader ? shader->shadingEngine() : nullptr;

	if( !shadingEngine )
	{
		return;
	}

	shadingEngine->hash( h );
	h.append(iterationGridPlug()->hash() );
	h.append( modePlug()->hash() );

	h.append( inPlug()->fullTransformHash( path ) );
}

static const IECore::InternedString g_world("world");

IECore::ConstObjectPtr OSLVDB::computeProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::ConstObjectPtr inputObject ) const
{
	const IECoreVDB::VDBObject *vdbObject = runTimeCast<const IECoreVDB::VDBObject>( inputObject.get() );

	if( !vdbObject )
	{
		return inputObject;
	}

	const OSLShader *shader = runTimeCast<const OSLShader>( shaderPlug()->source()->node() );
	ConstShadingEnginePtr shadingEngine = shader ? shader->shadingEngine() : nullptr;

	if( !shadingEngine )
	{
		return inputObject;
	}

	auto iterationGrid = vdbObject->findGrid(iterationGridPlug()->getValue() );
	if ( !iterationGrid )
	{
		return inputObject;
	}

	CellIterationMode iterationMode = modePlug()->getValue() ? CellIterationMode::AllCells : CellIterationMode::ActiveCells;

	CompoundDataPtr shadingPoints = prepareShadingPoints( vdbObject, iterationGrid, shadingEngine.get(), iterationMode );

	ShadingEngine::Transforms transforms;

	transforms[ g_world ] = ShadingEngine::Transform( inPlug()->fullTransform( path ) );

	CompoundDataPtr shadedPoints = shadingEngine->shade( shadingPoints.get(), transforms );

	auto newVDBObject = vdbObject->copy();

	for( CompoundDataMap::const_iterator it = shadedPoints->readable().begin(), eIt = shadedPoints->readable().end(); it != eIt; ++it )
	{
		// Ignore the output color closure as the debug closures are used to define what is 'exported' from the shader
		if( it->first != "Ci" )
		{
			auto newGrid = convertFrom( it->second, iterationGrid, iterationMode );
			if ( newGrid )
			{
				newGrid->setName( it->first );
				newVDBObject->insertGrid( newGrid );
			}
		}
	}

	return newVDBObject;
}

void OSLVDB::updateOutputGrids()
{
    // Disable undo for the actions we perform, because anything that can
    // trigger an update is undoable itself, and we will take care of everything as a whole
    // when we are undone.
    UndoScope undoDisabler( scriptNode(), UndoScope::Disabled );

    // Currently the OSLCode node will recompile every time an input is added.
    // We're hoping in the future to avoid doing this until the network is actually needed,
    // but in the meantime, we can save some time by emptying the code first, so that at least
    // all the redundant recompiles are of shorter code.
    oslCode()->codePlug()->setValue( "" );

    oslCode()->parametersPlug()->clearChildren();

    std::string code = "closure color out = 0;\n";

    for( NameValuePlugIterator inputPlug( outputGridsPlug() ); !inputPlug.done(); ++inputPlug )
    {
        std::string prefix = "";
        BoolPlug* enabledPlug = (*inputPlug)->enabledPlug();
        if( enabledPlug )
        {
            IntPlugPtr codeEnablePlug = new IntPlug( "enable" );
            oslCode()->parametersPlug()->addChild( codeEnablePlug );
            codeEnablePlug->setInput( enabledPlug );
            prefix = "if( " + codeEnablePlug->getName().string() + " ) ";
        }

        Plug *valuePlug = (*inputPlug)->valuePlug();

        if( valuePlug->typeId() == ClosurePlug::staticTypeId() )
        {
            // Closures are a special case that doesn't need a wrapper function
            ClosurePlugPtr codeClosurePlug = new ClosurePlug( "closureIn" );
            oslCode()->parametersPlug()->addChild( codeClosurePlug );
            codeClosurePlug->setInput( valuePlug );

            code += prefix + "out = out + " + codeClosurePlug->getName().string() + ";\n";
            continue;
        }

        std::string outFunction;
        PlugPtr codeValuePlug;
        const Gaffer::TypeId valueType = (Gaffer::TypeId)valuePlug->typeId();
        switch( (int)valueType )
        {
            case FloatPlugTypeId :
                codeValuePlug = new FloatPlug( "value" );
                outFunction = "outFloat";
                break;
            case IntPlugTypeId :
                codeValuePlug = new IntPlug( "value" );
                outFunction = "outInt";
                break;
            case Color3fPlugTypeId :
                codeValuePlug = new Color3fPlug( "value" );
                outFunction = "outColor";
                break;
            case V3fPlugTypeId :
                codeValuePlug = new V3fPlug( "value" );
                {
                    V3fPlug *v3fPlug = runTimeCast<V3fPlug>( valuePlug );
                    if( v3fPlug->interpretation() == GeometricData::Point )
                    {
                        outFunction = "outPoint";
                    }
                    else if( v3fPlug->interpretation() == GeometricData::Normal )
                    {
                        outFunction = "outNormal";
                    }
                    else if( v3fPlug->interpretation() == GeometricData::UV )
                    {
                        outFunction = "outUV";
                    }
                    else
                    {
                        outFunction = "outVector";
                    }
                }
                break;
            case M44fPlugTypeId :
                codeValuePlug = new M44fPlug( "value" );
                outFunction = "outMatrix";
                break;
            case StringPlugTypeId :
                codeValuePlug = new StringPlug( "value" );
                outFunction = "outString";
                break;
        }

        if( codeValuePlug )
        {

            StringPlugPtr codeNamePlug = new StringPlug( "name" );
            oslCode()->parametersPlug()->addChild( codeNamePlug );
            codeNamePlug->setInput( (*inputPlug)->namePlug() );

            oslCode()->parametersPlug()->addChild( codeValuePlug );
            codeValuePlug->setInput( valuePlug );

            code += prefix + "out = out + " + outFunction + "( " + codeNamePlug->getName().string() + ", "
                    + codeValuePlug->getName().string() + ");\n";
            continue;
        }

        IECore::msg( IECore::Msg::Warning, "OSLObject::updatePrimitiveVariables",
                     "Could not create primitive variable from plug: " + (*inputPlug)->fullName()
        );
    }
    code += "Ci = out;\n";

    oslCode()->codePlug()->setValue( code );
}

void OSLVDB::outputGridAdded( const Gaffer::GraphComponent *parent, Gaffer::GraphComponent *child )
{
    updateOutputGrids();
}

void OSLVDB::outputGridRemoved( const Gaffer::GraphComponent *parent, Gaffer::GraphComponent *child )
{
    updateOutputGrids();
}