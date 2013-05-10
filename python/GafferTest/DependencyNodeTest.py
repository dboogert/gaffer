##########################################################################
#  
#  Copyright (c) 2011-2012, John Haddon. All rights reserved.
#  Copyright (c) 2011-2013, Image Engine Design Inc. All rights reserved.
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
#      * Neither the name of John Haddon nor the names of
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

import unittest

import Gaffer
import GafferTest

class DependencyNodeTest( GafferTest.TestCase ) :
											
	def testDirtyOnDisconnect( self ) :
	
		n1 = GafferTest.AddNode( "n1" )
		n2 = GafferTest.AddNode( "n2" )
		
		n1["op1"].setValue( 2 )
		n1["op2"].setValue( 3 )
		
		dirtied = GafferTest.CapturingSlot( n2.plugDirtiedSignal() )
		set = GafferTest.CapturingSlot( n2.plugSetSignal() )
		n2["op1"].setInput( n1["sum"] )
				
		self.assertEqual( len( set ), 0 )
		self.assertEqual( len( dirtied ), 2 )
		self.failUnless( dirtied[0][0].isSame( n2["op1"] ) )
		self.failUnless( dirtied[1][0].isSame( n2["sum"] ) )

		n2["op1"].setInput( None )
		
		self.assertEqual( len( set ), 1 )
		self.failUnless( set[0][0].isSame( n2["op1"] ) )
		self.assertEqual( len( dirtied ), 4 )
		self.failUnless( dirtied[2][0].isSame( n2["op1"] ) )
		self.failUnless( dirtied[3][0].isSame( n2["sum"] ) )
		
	def testDirtyPropagationForCompoundPlugs( self ) :
	
		class CompoundOut( Gaffer.DependencyNode ) :
		
			def __init__( self, name="CompoundOut" ) :
			
				Gaffer.DependencyNode.__init__( self, name )
			
				self["in"] = Gaffer.IntPlug()
				self["out"] = Gaffer.CompoundPlug( direction = Gaffer.Plug.Direction.Out )
				self["out"]["one"] = Gaffer.IntPlug( direction = Gaffer.Plug.Direction.Out )
				self["out"]["two"] = Gaffer.IntPlug( direction = Gaffer.Plug.Direction.Out )
				
				self["behaveBadly"] = Gaffer.BoolPlug( defaultValue = False )
				
			def affects( self, input ) :
			
				outputs = Gaffer.DependencyNode.affects( self, input )
			
				if input.isSame( self["in"] ) :
					if self["behaveBadly"].getValue() :
						# we're not allowed to return a CompoundPlug in affects() - we're
						# just doing it here to make sure we can see that the error is detected.
						outputs.append( self["out"] )
					else :
						# to behave well we must list all leaf level children explicitly.
						outputs.extend( self["out"].children() )						
					
				return outputs
					
		class CompoundIn( Gaffer.DependencyNode ) :
		
			def __init__( self, name="CompoundIn" ) :
			
				Gaffer.DependencyNode.__init__( self, name )

				self["in"] = Gaffer.CompoundPlug()
				self["in"]["one"] = Gaffer.IntPlug()
				self["in"]["two"] = Gaffer.IntPlug()
				
				self["out"] = Gaffer.IntPlug( direction = Gaffer.Plug.Direction.Out )
				
			def affects( self, input ) :
			
				# affects should never be called with a CompoundPlug - only
				# leaf level plugs.
				assert( not input.isSame( self["in"] ) )
			
				if self["in"].isAncestorOf( input ) :
					return [ self["out"] ]
					
				return []
		
		src = CompoundOut()
		dst = CompoundIn()
		
		dst["in"].setInput( src["out"] )		
		
		srcDirtied = GafferTest.CapturingSlot( src.plugDirtiedSignal() )
		dstDirtied = GafferTest.CapturingSlot( dst.plugDirtiedSignal() )
		
		src["behaveBadly"].setValue( True )
		self.assertRaises( RuntimeError, src["in"].setValue, 10 )
		
		src["behaveBadly"].setValue( False )		
		src["in"].setValue( 20 )
				
		srcDirtiedNames = set( [ x[0].fullName() for x in srcDirtied ] )
		
		self.assertEqual( len( srcDirtiedNames ), 3 )
		
		self.assertTrue( "CompoundOut.out.one" in srcDirtiedNames )
		self.assertTrue( "CompoundOut.out.two" in srcDirtiedNames )
		self.assertTrue( "CompoundOut.out" in srcDirtiedNames )
		
		dstDirtiedNames = set( [ x[0].fullName() for x in dstDirtied ] )

		self.assertEqual( len( dstDirtiedNames ), 4 )
		self.assertTrue( "CompoundIn.in.one" in dstDirtiedNames )
		self.assertTrue( "CompoundIn.in.two" in dstDirtiedNames )
		self.assertTrue( "CompoundIn.in" in dstDirtiedNames )
		self.assertTrue( "CompoundIn.out" in dstDirtiedNames )
				
	def testAffectsRejectsCompoundPlugs( self ) :
	
		n = GafferTest.CompoundPlugNode()
		
		self.assertRaises( RuntimeError, n.affects, n["p"] )
	
	def testAffectsWorksWithPlugs( self ) :
	
		# check that we can propagate dirtiness for simple Plugs, and
		# not just ValuePlugs.
	
		class SimpleDependencyNode( Gaffer.DependencyNode ) :
	
			def __init__( self, name="PassThrough", inputs={}, dynamicPlugs=() ) :
	
				Gaffer.DependencyNode.__init__( self, name )

				self.addChild( Gaffer.Plug( "in", Gaffer.Plug.Direction.In ) )
				self.addChild( Gaffer.Plug( "out", Gaffer.Plug.Direction.Out ) )

			def affects( self, input ) :

				assert( input.isSame( self["in"] ) )	
				return [ self["out"] ]
				
		s1 = SimpleDependencyNode()
		s2 = SimpleDependencyNode()
		
		cs = GafferTest.CapturingSlot( s2.plugDirtiedSignal() )
		
		s2["in"].setInput( s1["out"] )
		
		self.assertEqual( len( cs ), 2 )
		self.assertTrue( cs[0][0].isSame( s2["in"] ) )
		self.assertTrue( cs[1][0].isSame( s2["out"] ) )
		
if __name__ == "__main__":
	unittest.main()
