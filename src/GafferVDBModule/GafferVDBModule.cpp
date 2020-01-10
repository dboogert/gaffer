//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2017, Image Engine Design. All rights reserved.
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
//      * Neither the name of Image Engine Design nor the names of
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

#include "boost/python.hpp"

#include "GafferVDB/OffsetLevelSet.h"
#include "GafferVDB/LevelSetToMesh.h"
#include "GafferVDB/MeshToLevelSet.h"
#include "GafferVDB/PointsGridToPoints.h"
#include "GafferVDB/DeleteGrids.h"
#include "GafferVDB/ScatterPoints.h"
#include "GafferVDB/AdvectGrids.h"
#include "GafferVDB/MathOpGrids.h"
#include "GafferVDB/StatisticsGrids.h"
#include "GafferVDB/CSGLevelSets.h"
#include "GafferVDB/TransformGrids.h"
#include "GafferVDB/PointsToLevelSet.h"
#include "GafferVDB/VDBObject.h"
#include "GafferVDB/SampleGrids.h"
#include "GafferVDB/FilterGrids.h"
#include "GafferVDB/MeasureLevelSet.h"
#include "GafferVDB/FilterLevelSet.h"
#include "GafferVDB/VolumeToSpheres.h"
#include "GafferVDB/ClipGrids.h"
#include "GafferVDB/LevelSetToFog.h"
#include "GafferVDB/SegmentLevelSets.h"
#include "GafferVDB/IntersectLevelSet.h"
#include "GafferVDB/MorphLevelSets.h"
#include "GafferVDB/CompositeGrids.h"
#include "GafferVDB/PlatonicLevelSet.h"

#include "GafferBindings/DependencyNodeBinding.h"

using namespace boost::python;
using namespace GafferVDB;

BOOST_PYTHON_MODULE( _GafferVDB )
{

	GafferBindings::DependencyNodeClass<MeshToLevelSet>();
	GafferBindings::DependencyNodeClass<LevelSetToMesh>();
	GafferBindings::DependencyNodeClass<OffsetLevelSet>();
	GafferBindings::DependencyNodeClass<PointsGridToPoints>();
	GafferBindings::DependencyNodeClass<DeleteGrids>();
	GafferBindings::DependencyNodeClass<ScatterPoints>();
    GafferBindings::DependencyNodeClass<AdvectGrids>();
	GafferBindings::DependencyNodeClass<MathOpGrids>();
	GafferBindings::DependencyNodeClass<StatisticsGrids>();
	GafferBindings::DependencyNodeClass<CSGLevelSets>();
	GafferBindings::DependencyNodeClass<TransformGrids>();
	GafferBindings::DependencyNodeClass<PointsToLevelSet>();
	GafferBindings::DependencyNodeClass<VDBObject>();
	GafferBindings::DependencyNodeClass<SampleGrids>();
	GafferBindings::DependencyNodeClass<FilterGrids>();
	GafferBindings::DependencyNodeClass<MeasureLevelSet>();
	GafferBindings::DependencyNodeClass<FilterLevelSet>();
	GafferBindings::DependencyNodeClass<VolumeToSpheres>();
	GafferBindings::DependencyNodeClass<ClipGrids>();
	GafferBindings::DependencyNodeClass<LevelSetToFog>();
	GafferBindings::DependencyNodeClass<SegmentLevelSets>();
    GafferBindings::DependencyNodeClass<IntersectLevelSet>();
    GafferBindings::DependencyNodeClass<MorphLevelSets>();
    GafferBindings::DependencyNodeClass<CompositeGrids>();
    GafferBindings::DependencyNodeClass<PlatonicLevelSet>();
}
