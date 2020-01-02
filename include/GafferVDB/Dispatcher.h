//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2018, Don Boogert. All rights reserved.
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
//      * Neither the name ofDon Boogert nor the names of
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

#ifndef GAFFERVDB_DISPATCHER_H
#define GAFFERVDB_DISPATCHER_H

#include "GafferVDB/Export.h"

#include "Gaffer/Context.h"

#include <openvdb/openvdb.h>

namespace GafferVDB
{

    template<template<typename> class F, typename N, typename R>
    class ScalarGridDispatcher
    {
    public:
        ScalarGridDispatcher(const N* node, const Gaffer::Context* context ) : node( node ), context( context ) {}

        R operator()( openvdb::GridBase::Ptr grid ) const
        {
            if (grid->valueType() == openvdb::typeNameAsString<double>())
            {
                F<openvdb::DoubleGrid> f;
                openvdb::DoubleGrid::Ptr typedGrid = openvdb::GridBase::grid<openvdb::DoubleGrid>(grid);
                return f(typedGrid, node, context );

            } else if (grid->valueType() == openvdb::typeNameAsString<float>())
            {
                F<openvdb::FloatGrid> f;
                openvdb::FloatGrid::Ptr typedGrid = openvdb::GridBase::grid<openvdb::FloatGrid>(grid);
                return f(typedGrid, node, context );
            }

            return R();

        }

        R operator()( openvdb::GridBase::ConstPtr grid ) const
        {
            if ( grid->valueType() == openvdb::typeNameAsString<double>() )
            {
                F<openvdb::DoubleGrid> f;

                openvdb::DoubleGrid::ConstPtr constTypedGrid = openvdb::GridBase::constGrid<openvdb::DoubleGrid>( grid );
                return f( constTypedGrid, node, context );
            }
            else if ( grid->valueType() == openvdb::typeNameAsString<float>() )
            {
                F<openvdb::FloatGrid> f;

                openvdb::FloatGrid::ConstPtr constTypedGrid = openvdb::GridBase::constGrid<openvdb::FloatGrid>( grid );
                return f( constTypedGrid, node, context );
            }

            return R();

        }

        const N* node;
        const Gaffer::Context* context;
    };


} // namespace GafferVDB

#endif // GAFFERVDB_DISPATCHER_H




