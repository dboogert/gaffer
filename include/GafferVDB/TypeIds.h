//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2017, John Haddon. All rights reserved.
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

#ifndef GAFFERVDB_TYPEIDS_H
#define GAFFERVDB_TYPEIDS_H

namespace GafferVDB
{

enum TypeId
{
	ClipTypeId = 110950,
	VDBObjectTypeId = 110951,
	LevelSetMorphTypeId = 110952,
	MeshToLevelSetTypeId = 110953,
	LevelSetToMeshTypeId = 110954,
	LevelSetOffsetTypeId = 110955,
	PointsGridToPointsId = 110956,
	DeleteGridsTypeId = 110957,
	ScatterPointsTypeId = 110958,
	AdvectGridsTypeId = 110959,
	MathOpTypeId = 110960,
	StatisticsTypeId = 110961,
	CSGGridsTypeId = 110962,
	TransformGridsTypeId = 110963,
	LevelSetFractureTypeId = 110964,
	PointsToLevelSetTypeId = 110965,
	SampleTypeId = 110966,
	FilterGridsTypeId = 110967,
	LevelSetMeasureTypeId = 110968,
	LevelSetFilterTypeId = 110969,
	VolumeToSpheresTypeId = 110970,
    LevelSetToFogTypeId = 110971,
    SegmentGridsTypeId = 110972,
	LastTypeId = 110974
};

} // namespace GafferVDB

#endif // GAFFERVDB_TYPEIDS_H
