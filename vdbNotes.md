
* General or grid type context menu?
- 

* [LF][1 VDB input] DeleteGrids - update to Deformer 
    * Types are unchanged
* [LF][1 VDB input] ScatterPoints - update to Deformer
    * S, S-L -> Points
    * grey out non operation parameters based on mode
    * Non uniform -> probability
    * uniform -> count
    * VDB points output not working
* [?][2 VDB inputs] AdvectGrids - update to Deformer
    * T # V -> T where T is (S, V, S-L) 
    * Use UI from CopyPrimitive Variables
    * Diagnostics for other unsupported grid types
    * add suffix plug (consider if this is worth it?)
    * Would be really handy to generate grids in code for testing
    * Does it work on LevelSet or Fog?       
* [?] [1 VDB input] MathOpGrids
    * Works on scalars or Vectors 
        * Weird combination of grad(S) -> V , laplacian(S) -> S, div(V) -> S, curl(V) ->V , curvature(S-L) -> S
* [LF][1 VDB input] StatisticsGrids
    * (S, V, S-L) 
    * rename attribute to statistics:[grid]:[stat]
* [L] [2 VDB inputs] CSGLevelSets
    * (S-L) # (S-L) -> (S-L)
        * needs to work with other scalar grids
    * PROBLEM - mesh to level set - the grid isn't visible because of the grid rendering
* [LF][1 VDB input] Transform  
    * looks like there is a problem transforming level sets (create a LS from a cube)
        * Also a problem with fog volumes
* [L][1 VDB input + 1 points input] PointsToLevelSet
    * add radius parameter
    * NEED an empty grid of known resolution & type
    * Add trails option                    

* [L] Primitives (LevelSetPlatonic.h)
    Platonic
* [L] Sphere (LevelSetSphere.h)
      

drop down for compatible grid names

UI For VDBObject node (grid type drop down, tool tips for other parameters)
Render (input camera & VDB and grid) output image
Interupt long running operations

How can i make sure there is always something visualised

points to level set 
    - attribute transfer from particles to surface (leave this to v2.0)
    - S-L # Points -> S-L
    - double SDF!

SegmentLevelSets
    - double SDF
    
VolumeToSpheres
    - works on Scalar field and LS
    - double type

CSGLevelSets
    - grid selector UI
    - separate grid name for 2nd input ?
    - tool tips
    
ScatterPointsInVolume 
    - float / double scalar grid
    - grid selector
    
    
Nodes to make:

   AdvectGrids
   CompositeGrids
   IntersectGrids
   FractureLevelSets
   MorphLevelSets
   LevelSetPrimitive
   SetBackgroundGrids
   
   DiagnosticsGrids
   PlatonticLevelSet
   SphereLevelSet
   
   CreateMaskGrid
    
   OffsetLevelSet 
        Interrupt
        outputGrid   
  
  PruneGrids
  PotentialFlow

Update to Deformer base
Discuss 2nd input scene locattions 
use transform of vdb / primitive in all nodes (remove the need to freeze transforms)
FilterGrids - disable doesn't pass through input vdb
grid naming attribute ${grid} context variable 
consistent grid naming using a context variable evaluation

Merge this goodness into my VDB work : dboogert:OSLVDB

Add mask to filtering and morphology            
Copy grids from one VDB into another
Fracture levelset
Advect points
LevelSetAdvection                              
Trace rays against a level set / Volume
    * https://www.openvdb.org/documentation/doxygen/RayIntersector_8h.html                                                                                                                                                                                                                                                                                                                                                                                      
Morph
    * https://www.openvdb.org/documentation/doxygen/LevelSetMorph_8h.html
cast from different grid types

Check nodes to see what support masks

how do I optimize bounds????!?!
VDB Scene Writer
Use latest base classes 

Build 
    Missing inksapce doesn't give a decent warning in the cmake build 
    
    
DeleteChannelsUI.py
    - "plugValueWidget:type", "GafferImageUI.ChannelMaskPlugValueWidget",
    
    ChannelMaskPlugValueWidget ->  GafferUI.PlugValueWidget
    
    all
    clear
    [gridname] [fog | levelset] [type]  
    
    How can a simple context menu be assigned 