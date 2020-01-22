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

#ifndef GAFFERUI_RENDERSTATE_H
#define GAFFERUI_RENDERSTATE_H

#include "OpenEXR/ImathMatrix.h"

#include <array>
#include <vector>

namespace GafferUI
{

class RenderState
{
	public:

		RenderState();
		~RenderState();

		enum class Stack : unsigned char
		{
			PROJECTION,
			MODELVIEW,
			TEXTURE,
			MAX
		};

		void setClearColor( float r, float g, float b, float a );
		void setClearDepth( float depth );

		void clear( bool color, bool depth ) const;

		void set( Stack stack, const Imath::M44f& matrix );
		void transform( Stack stack, const Imath::M44f& matrix );
		void push( Stack stack );
		void pop( Stack stack );

		static RenderState& Get();

	private:
		float m_clearR;
		float m_clearG;
		float m_clearB;
		float m_clearA;
		float m_clearDepth;

		std::array<std::vector<Imath::M44f>, static_cast<int> (Stack::MAX)> m_matrices;
};

} // namespace GafferUI

#endif // GAFFERUI_BACKDROPNODEGADGET_H