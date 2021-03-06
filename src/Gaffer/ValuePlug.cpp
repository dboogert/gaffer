//////////////////////////////////////////////////////////////////////////
//  
//  Copyright (c) 2011-2012, John Haddon. All rights reserved.
//  Copyright (c) 2011-2013, Image Engine Design Inc. All rights reserved.
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

#include <stack>

#include "tbb/enumerable_thread_specific.h"

#include "boost/bind.hpp"
#include "boost/format.hpp"

#include "IECore/LRUCache.h"

#include "Gaffer/ValuePlug.h"
#include "Gaffer/ComputeNode.h"
#include "Gaffer/Context.h"
#include "Gaffer/Action.h"

using namespace Gaffer;

//////////////////////////////////////////////////////////////////////////
// Computation implementation
// The computation class is responsible for managing the transient storage
// necessary for computed results, and for managing the call to Node::compute().
// One day it may also be responsible for managing a cache of previous
// results and maybe even shipping some computations off over the network
// to a special computation server of some sort.
//////////////////////////////////////////////////////////////////////////

class ValuePlug::Computation
{
	
	public :
	
		Computation( const ValuePlug *resultPlug )
			:	m_resultPlug( resultPlug ), m_resultValue( NULL )
		{
			g_threadComputations.local().push( this );
		}
		
		~Computation()
		{
			g_threadComputations.local().pop();
		}

		const ValuePlug *resultPlug() const
		{
			return m_resultPlug;
		}

		IECore::ConstObjectPtr compute()
		{
			// decide whether or not to use the cache. even if
			// the result plug has the Cacheable flag set, we disable
			// caching if it gets its value from a direct input which
			// does not have the Cacheable flag set.
			bool cacheable = true;
			const ValuePlug *p = m_resultPlug;
			while( p )
			{
				if( !p->getFlags( Plug::Cacheable ) )
				{
					cacheable = false;
					break;
				}
				p = p->getInput<ValuePlug>();
			}
						
			// do the cache lookup/computation.						
			if( cacheable )
			{
				IECore::MurmurHash hash = m_resultPlug->hash();
				m_resultValue = g_valueCache.get( hash );
				if( !m_resultValue )
				{
					computeOrSetFromInput();
					g_valueCache.set( hash, m_resultValue, m_resultValue->memoryUsage() );
				}
			}
			else
			{
				// plug has requested no caching, so we compute from scratch every
				// time.
				computeOrSetFromInput();
			}
			
			return m_resultValue;
		}
				
		static void receiveResult( const ValuePlug *plug, IECore::ConstObjectPtr result )
		{
			Computation *computation = Computation::current();
			if( !computation )
			{
				throw IECore::Exception( boost::str( boost::format( "Cannot set value for plug \"%s\" except during computation." ) % plug->fullName() ) );					
			}
			
			if( computation->m_resultPlug != plug )
			{
				throw IECore::Exception( boost::str( boost::format( "Cannot set value for plug \"%s\" during computation for plug \"%s\"." ) % plug->fullName() % computation->m_resultPlug->fullName() ) );						
			}
			
			computation->m_resultValue = result;
		}
		
		static Computation *current()
		{
			ComputationStack &s = g_threadComputations.local();
			if( !s.size() )
			{
				return 0;
			}
			return s.top();
		}
	
		static size_t getCacheMemoryLimit()
		{
			return g_valueCache.getMaxCost();
		}
		
		static void setCacheMemoryLimit( size_t bytes )
		{
			return g_valueCache.setMaxCost( bytes );
		}
	
		static size_t cacheMemoryUsage()
		{
			return g_valueCache.currentCost();
		}
		
	private :
	
		// Fills in m_resultValue by calling ComputeNode::compute() or ValuePlug::setFrom().
		// Throws if the result was not successfully retrieved.
		void computeOrSetFromInput()
		{
			if( const ValuePlug *input = m_resultPlug->getInput<ValuePlug>() )
			{
				// cast is ok, because we know that the resulting setValue() call won't
				// actually modify the plug, but will just place the value in our m_resultValue.
				const_cast<ValuePlug *>( m_resultPlug )->setFrom( input );
			}
			else
			{
				const ComputeNode *n = m_resultPlug->ancestor<ComputeNode>();
				if( !n )
				{
					throw IECore::Exception( boost::str( boost::format( "Unable to compute value for Plug \"%s\" as it has no ComputeNode." ) % m_resultPlug->fullName() ) );			
				}
				// cast is ok - see comment above.
				n->compute( const_cast<ValuePlug *>( m_resultPlug ), Context::current() );
			}
			
			// the calls above should cause setValue() to be called on the result plug, which in
			// turn will call ValuePlug::setObjectValue(), which will then store the result in
			// the current computation by calling receiveResult(). If that hasn't happened then
			// something has gone wrong and we should complain about it.
			if( !m_resultValue )
			{
				throw IECore::Exception( boost::str( boost::format( "Value for Plug \"%s\" not set as expected." ) % m_resultPlug->fullName() ) );
			}
		}
	
		const ValuePlug *m_resultPlug;
		IECore::ConstObjectPtr m_resultValue;

		typedef std::stack<Computation *> ComputationStack;
		typedef tbb::enumerable_thread_specific<ComputationStack> ThreadSpecificComputationStack;
		static ThreadSpecificComputationStack g_threadComputations;
		
		static IECore::ObjectPtr nullGetter( const IECore::MurmurHash &h, size_t &cost )
		{
			cost = 0;
			return NULL;
		}
		
		typedef IECore::LRUCache<IECore::MurmurHash, IECore::ConstObjectPtr> ValueCache;
		static ValueCache g_valueCache;
		
};

ValuePlug::Computation::ThreadSpecificComputationStack ValuePlug::Computation::g_threadComputations;
ValuePlug::Computation::ValueCache ValuePlug::Computation::g_valueCache( nullGetter, 1024 * 1024 * 500 );

//////////////////////////////////////////////////////////////////////////
// SetValueAction implementation
//////////////////////////////////////////////////////////////////////////

class ValuePlug::SetValueAction : public Gaffer::Action
{

	public :
	
		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( Gaffer::ValuePlug::SetValueAction, SetValueActionTypeId, Gaffer::Action );

		SetValueAction( ValuePlugPtr plug, IECore::ConstObjectPtr value )
			:	m_plug( plug ), m_doValue( value ), m_undoValue( plug->m_staticValue )
		{
		}

	protected :

		virtual GraphComponent *subject() const
		{
			return m_plug.get();
		}
		
		virtual void doAction()
		{
			m_plug->setValueInternal( m_doValue, true );
		}
		
		virtual void undoAction()
		{
			m_plug->setValueInternal( m_undoValue, true );
		}

		virtual bool canMerge( const Action *other ) const
		{
			if( !Action::canMerge( other ) )
			{
				return false;
			}
			const SetValueAction *setValueAction = IECore::runTimeCast<const SetValueAction>( other );
			return setValueAction && setValueAction->m_plug == m_plug;
		}
		
		virtual void merge( const Action *other )
		{
			const SetValueAction *setValueAction = static_cast<const SetValueAction *>( other );
			m_doValue = setValueAction->m_doValue;
		}

	private :

		ValuePlugPtr m_plug;
		IECore::ConstObjectPtr m_doValue;
		IECore::ConstObjectPtr m_undoValue;
		
};

IE_CORE_DEFINERUNTIMETYPED( ValuePlug::SetValueAction );

//////////////////////////////////////////////////////////////////////////
// ValuePlug implementation
//////////////////////////////////////////////////////////////////////////

IE_CORE_DEFINERUNTIMETYPED( ValuePlug );

/// \todo We may want to avoid repeatedly storing copies of the same default value
/// passed to this function. Perhaps by having a central map of unique values here,
/// or by doing it more intelligently in the derived classes (where we could avoid
/// even creating the values before figuring out if we've already got them somewhere).
ValuePlug::ValuePlug( const std::string &name, Direction direction,
	IECore::ConstObjectPtr initialValue, unsigned flags )
	:	Plug( name, direction, flags ), m_staticValue( initialValue )
{
	assert( m_staticValue );
}

ValuePlug::ValuePlug( const std::string &name, Direction direction, unsigned flags )
	:	Plug( name, direction, flags ), m_staticValue( 0 )
{
}

ValuePlug::~ValuePlug()
{
}

bool ValuePlug::acceptsInput( const Plug *input ) const
{
	if( !Plug::acceptsInput( input ) )
	{
		return false;
	}
	if( input )
	{
		return input->isInstanceOf( staticTypeId() );
	}
	return true;
}

void ValuePlug::setInput( PlugPtr input )
{
	if( input.get() == getInput<Plug>() )
	{
		return;
	}
	
	// set value back to what it was before
	// we received a connection. we do that
	// before calling Plug::setInput, so that
	// we've got our new state set correctly before
	// the dirty signal is emitted. we don't emit
	// in the setValueInternal call, because we don't
	// want to double up on the signals that the Plug
	// is emitting for us in Plug::setInput().
	if( !input )
	{
		setValueInternal( m_staticValue, false );
	}
	
	Plug::setInput( input );
}

bool ValuePlug::settable() const
{
	if( getFlags( ReadOnly ) )
	{
		return false;
	}
	
	if( getInput<Plug>() )
	{
		return false;
	}
	
	if( Computation *c = Computation::current() )
	{
		return c->resultPlug() == this;
	}
	else
	{
		return direction() == Plug::In;
	}
}

IECore::MurmurHash ValuePlug::hash() const
{
	IECore::MurmurHash h;
	const ValuePlug *input = getInput<ValuePlug>();
	if( input )
	{
		if( input->typeId() == typeId() )
		{
			// we can assume that setFrom( input ) would perform no
			// conversion on the value, so by sharing hashes we also
			// get to share cache entries.
			h = input->hash();
		}
		else
		{
			// it would be unsafe to assume we can share cache entries,
			// because conversion is probably performed by setFrom( input ).
			// hash in a little extra something to represent the conversion
			// and break apart the cache entries.
			h = input->hash();
			h.append( input->typeId() );
			h.append( typeId() );
		}
	}
	else
	{
		if( direction() == Plug::In )
		{
			h = m_staticValue->hash();
		}
		else
		{
			const ComputeNode *n = ancestor<ComputeNode>();
			if( n )
			{
				IECore::MurmurHash emptyHash;
				n->hash( this, Context::current(), h );
				if( h == emptyHash )
				{
					throw IECore::Exception( boost::str( boost::format( "ComputeNode::hash() not implemented for Plug \"%s\"." ) % fullName() ) );			
				}
			}
			else
			{
				h = m_staticValue->hash();
			}
		}
	}
	
	return h;
}

void ValuePlug::hash( IECore::MurmurHash &h ) const
{
	h.append( hash() );
}

IECore::ConstObjectPtr ValuePlug::getObjectValue() const
{
	if( !getInput<Plug>() )
	{
		if( direction()==In || !ancestor<ComputeNode>() )
		{
			// no input connection, and no means of computing
			// a value. there can only ever be a single value,
			// which is stored directly on the plug.
			return m_staticValue;
		}
	}
	
	// an plug with an input connection or an output plug on a ComputeNode. there can be many values -
	// one per context. the computation class is responsible for providing storage for the result
	// and also actually managing the computation.
	Computation computation( this );
	return computation.compute();
}

void ValuePlug::setObjectValue( IECore::ConstObjectPtr value )
{
	bool haveInput = getInput<Plug>();
	if( direction()==In && !haveInput )
	{
		// input plug with no input connection. there can only ever be a single value,
		// which we store directly on the plug. when setting this we need to take care
		// of undo, and also of triggering the plugValueSet signal and propagating the
		// plugDirtiedSignal.
		
		if( getFlags( ReadOnly ) )
		{
			// We don't allow static values to be set on read only plugs, so we throw.
			// Note that it is perfectly acceptable to call setValue() on a read only
			// plug during a computation because the result is not written onto the
			// plug itself, so we don't make the check in the case that we call
			// receiveResult() below. This allows plugs which have inputs to be made
			// read only after having their input set.
			throw IECore::Exception( boost::str( boost::format( "Cannot set value for read only plug \"%s\"" ) % fullName() ) );
		}
	
		if( value->isNotEqualTo( m_staticValue.get() ) )
		{
			Action::enact( new SetValueAction( this, value ) );
		}
		
		return;
	}

	// an input plug with an input connection or an output plug. we must be currently in a computation
	// triggered by getObjectValue() for a setObjectValue() call to be valid (receiveResult will check this).
	// we never trigger plugValueSet or plugDirtiedSignals during computation.
	Computation::receiveResult( this, value );
}

bool ValuePlug::inCompute() const
{
	return Computation::current();
}

void ValuePlug::setValueInternal( IECore::ConstObjectPtr value, bool propagateDirtiness )
{
	m_staticValue = value;
	
	// it is important that we emit the plug set signal before
	// we emit dirty signals. this is because the node may wish to
	// perform some internal setup when plugs are set, and listeners
	// on output plugs may pull to get new output values as soon as
	// the dirty signal is emitted.
	emitPlugSet();

	if( propagateDirtiness )
	{
		DependencyNode::propagateDirtiness( this );
	}
}

void ValuePlug::emitPlugSet()
{
	if( Node *n = node() )
	{
		ValuePlug *p = this;
		while( p )
		{
			n->plugSetSignal()( p );
			p = p->parent<ValuePlug>();
		}
	}
	
	// take a copy of the outputs, owning a reference - because who
	// knows what will be added and removed by the connected slots.
	std::vector<PlugPtr> o( outputs().begin(), outputs().end() );
	for( std::vector<PlugPtr>::const_iterator it=o.begin(), eIt=o.end(); it!=eIt; ++it )
	{
		if( ValuePlug *output = IECore::runTimeCast<ValuePlug>( it->get() ) )
		{
			output->emitPlugSet();
		}
	}
}

size_t ValuePlug::getCacheMemoryLimit()
{
	return Computation::getCacheMemoryLimit();
}

void ValuePlug::setCacheMemoryLimit( size_t bytes )
{
	Computation::setCacheMemoryLimit( bytes );
}

size_t ValuePlug::cacheMemoryUsage()
{
	return Computation::cacheMemoryUsage();
}
