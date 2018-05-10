##########################################################################
#
#  Copyright (c) 2018, Don Boogert. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#      * Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      * Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials provided with
#        the distribution.
#
#      * Neither the name of Don Boogert nor the names of
#        any other contributors to this software may be used to endorse or
#        promote products derived from this software without specific prior
#        written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##########################################################################

import Gaffer
import GafferScene

import IECoreScene

import GafferUI
import GafferSceneUI

# primvar name in spreadsheet
# pin spreadsheet on particular node
# padding around widget - terrible with list widgets
# heading style
# update if object changes
# Constant primvars

class PrimitiveSpreadsheet( GafferUI.NodeSetEditor ) :

    def __init__( self, scriptNode, sections = None, **kw ) :
        self.__targetPaths = []
        self.__outputScenePlugs = []

        self.__column = GafferUI.ListContainer( GafferUI.ListContainer.Orientation.Vertical, borderWidth=8, spacing=8 )

        self.__tabbedContainer = GafferUI.TabbedContainer()

        self.__column.append( self.__tabbedContainer )

        self.__dataWidgets = {}

        def listContainer( child ):
            l = GafferUI.ListContainer( GafferUI.ListContainer.Orientation.Vertical, borderWidth=8, spacing=8 )
            l.append( child )
            return l

        self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Constant] = GafferUI.VectorDataWidget(editable=False, header=True)
        self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Uniform] = GafferUI.VectorDataWidget(editable=False, header=True)
        self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Vertex] = GafferUI.VectorDataWidget(editable=False, header=True)
        self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Varying] = GafferUI.VectorDataWidget(editable=False, header=True)
        self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.FaceVarying] = GafferUI.VectorDataWidget(editable=False, header=True)

        self.__tabbedContainer.append( listContainer( self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Constant] ), "Constant" )
        self.__tabbedContainer.append( listContainer( self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Uniform] ), "Uniform" )
        self.__tabbedContainer.append( listContainer( self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Vertex] ), "Vertex" )
        self.__tabbedContainer.append( listContainer( self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Varying] ), "Varying" )
        self.__tabbedContainer.append( listContainer( self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.FaceVarying] ), "FaceVarying" )

        GafferUI.NodeSetEditor.__init__( self, self.__column , scriptNode, **kw )

        self._updateFromSet()


    def _updateFromSet( self ) :

        print "updateFromSet"

        GafferUI.NodeSetEditor._updateFromSet( self )

        node = self._lastAddedNode()
        print node

        if len(self.getNodeSet()):
            node =  self.getNodeSet()[-1]
            outputScenePlugs = [ p for p in node.children( GafferScene.ScenePlug ) if p.direction() == Gaffer.Plug.Direction.Out ]

            self.__outputScenePlugs = outputScenePlugs

        self.__updateLazily()

    def _updateFromContext( self, modifiedItems ) :
        print modifiedItems

        for item in modifiedItems :
            if not item.startswith( "ui:" ) or ( GafferSceneUI.ContextAlgo.affectsSelectedPaths( item ) and self.__targetPaths is None ) :
                self.__updateLazily()
                break

    @GafferUI.LazyMethod( deferUntilPlaybackStops = True )
    def __updateLazily( self ) :

        self.__update()

    def __update( self ) :

        # The SceneInspector's internal context is not necessarily bound at this point, which can lead to errors
        # if nodes in the graph are expecting special context variables, so we make sure it is:
        with self.getContext():
            self.__targetPaths = GafferSceneUI.ContextAlgo.getSelectedPaths( self.getContext() ).paths()
            print self.__targetPaths

            self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Constant].setData( None )
            self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Uniform].setData( None )
            self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Vertex].setData( None )
            self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.Varying].setData( None )
            self.__dataWidgets[IECoreScene.PrimitiveVariable.Interpolation.FaceVarying].setData( None )

            primVars = {}

            if len(self.__targetPaths) and len(self.__outputScenePlugs):
                obj = self.__outputScenePlugs[-1].object(self.__targetPaths[-1])
                if obj:

                    for primvarName in obj.keys():
                        if obj[primvarName].interpolation in primVars:
                            primVars[ obj[primvarName].interpolation ].append( obj[primvarName].data )
                        else:
                            primVars[ obj[primvarName].interpolation ] = [ obj[primvarName].data ]

                    for interpolation, dataArray in primVars.items():
                        if dataArray:
                            self.__dataWidgets[interpolation].setData( dataArray )


GafferUI.EditorWidget.registerType( "PrimitiveSpreadsheet", PrimitiveSpreadsheet )