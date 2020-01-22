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

#include "RenderState.h"

#include <GL/gl.h>

namespace GafferUI
{

RenderState::RenderState()
{
}

RenderState::~RenderState()
{
}

void RenderState::setClearColor( float r, float g, float b, float a )
{
	m_clearR = r;
	m_clearG = g;
	m_clearB = b;
	m_clearA = a;
}

void RenderState::RenderState::setClearDepth( float depth )
{
	m_clearDepth = depth;
}

void RenderState::clear( bool color, bool depth ) const
{
	glClearColor( m_clearR, m_clearG, m_clearB, m_clearA );
	glClearDepth( m_clearDepth );
	glClear( ( color ? GL_COLOR_BUFFER_BIT : 0 ) |  ( depth ? GL_DEPTH_BUFFER_BIT : 0 )  );
}

void RenderState::set( Stack stack, const Imath::M44f& matrix )
{
	*(m_matrices[(int)stack].rbegin()) = matrix;
}

void RenderState::transform( Stack stack, const Imath::M44f& matrix )
{
}

void RenderState::push( Stack stack )
{
}

void RenderState::pop( Stack stack )
{
}

RenderState& RenderState::Get()
{
	static RenderState renderState;
	return renderState;
}

} // GafferUI