//////////////////////////////////////////////////////////////////////////
//  
//  Copyright (c) 2013, John Haddon. All rights reserved.
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
//      * Neither the name of John Haddon nor the names of
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

#include "Gaffer/Box.h"

#include "GafferScene/ShaderSwitch.h"

#include "GafferOSL/OSLShader.h"
#include "GafferOSL/OSLImage.h"
#include "GafferOSL/OSLObject.h"

using namespace Imath;
using namespace IECore;
using namespace Gaffer;
using namespace GafferScene;
using namespace GafferOSL;

IE_CORE_DEFINERUNTIMETYPED( OSLObject );

size_t OSLObject::g_firstPlugIndex;

OSLObject::OSLObject( const std::string &name )
	:	SceneElementProcessor( name, Filter::NoMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new Plug( "shader" ) );
}

OSLObject::~OSLObject()
{
}

Gaffer::Plug *OSLObject::shaderPlug()
{
	return getChild<Plug>( g_firstPlugIndex );
}

const Gaffer::Plug *OSLObject::shaderPlug() const
{
	return getChild<Plug>( g_firstPlugIndex );
}

void OSLObject::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	SceneElementProcessor::affects( input, outputs );
	
	if( input == shaderPlug() )
	{
		outputs.push_back( outPlug()->objectPlug() );		
	}
	else if( input == outPlug()->objectPlug() )
	{
		outputs.push_back( outPlug()->boundPlug() );
	}
}

bool OSLObject::acceptsInput( const Gaffer::Plug *plug, const Gaffer::Plug *inputPlug ) const
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
		const Node *sourceNode = inputPlug->source<Plug>()->node();
		if( const OSLShader *shader = runTimeCast<const OSLShader>( sourceNode ) )
		{
			return shader->typePlug()->getValue() == "osl:surface";
		}
		else
		{
			// as for the GafferScene::ShaderAssignment, we accept Box and ShaderSwitch
			// inputs as an indirect means of later getting a connection to a Shader.
			if(
				runTimeCast<const Gaffer::Box>( sourceNode ) ||
				runTimeCast<const ShaderSwitch>( sourceNode )
			)
			{
				return true;
			}
		}
		return false;
	}
	
	return true;
}

bool OSLObject::processesBound() const
{
	return runTimeCast<const OSLShader>( shaderPlug()->source<Plug>()->node() );
}

void OSLObject::hashProcessedBound( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	hashProcessedObject( path, context, h );
}

Imath::Box3f OSLObject::computeProcessedBound( const ScenePath &path, const Gaffer::Context *context, const Imath::Box3f &inputBound ) const
{
	ConstObjectPtr object = outPlug()->objectPlug()->getValue();
	if( const Primitive *primitive = runTimeCast<const Primitive>( object.get() ) )
	{
		return primitive->bound();
	}
	return inputBound;
}

bool OSLObject::processesObject() const
{
	return runTimeCast<const OSLShader>( shaderPlug()->source<Plug>()->node() );
}

void OSLObject::hashProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	const OSLShader *shader = runTimeCast<const OSLShader>( shaderPlug()->source<Plug>()->node() );
	if( shader )
	{
		shader->stateHash( h );
	}
}

IECore::ConstObjectPtr OSLObject::computeProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::ConstObjectPtr inputObject ) const
{
	const Primitive *inputPrimitive = runTimeCast<const Primitive>( inputObject.get() );
	if( !inputPrimitive )
	{
		return inputObject;
	}

	if( !inputPrimitive->variableData<V3fVectorData>( "P", PrimitiveVariable::Vertex ) )
	{
		return inputObject;
	}

	ConstOSLShaderPtr shader = runTimeCast<const OSLShader>( shaderPlug()->source<Plug>()->node() );
	OSLRenderer::ConstShadingEnginePtr shadingEngine = shader ? shader->shadingEngine() : NULL;
	
	if( !shadingEngine )
	{
		return inputObject;	
	}
	
	CompoundDataPtr shadingPoints = new CompoundData;
	for( PrimitiveVariableMap::const_iterator it = inputPrimitive->variables.begin(), eIt = inputPrimitive->variables.end(); it != eIt; ++it )
	{
		if( it->second.interpolation == PrimitiveVariable::Vertex )
		{
			// cast is ok - we're only using it to be able to reference the data from the shadingPoints,
			// but nothing will modify the data itself.
			shadingPoints->writable()[it->first] = boost::const_pointer_cast<Data>( it->second.data );
		}
	}

	PrimitivePtr outputPrimitive = inputPrimitive->copy();

	CompoundDataPtr shadedPoints = shadingEngine->shade( shadingPoints.get() );
	for( CompoundDataMap::const_iterator it = shadedPoints->readable().begin(), eIt = shadedPoints->readable().end(); it != eIt; ++it )
	{
		if( it->first != "Ci" )
		{
			outputPrimitive->variables[it->first] = PrimitiveVariable( PrimitiveVariable::Vertex, it->second );
		}
	}
			
	return outputPrimitive;
}
