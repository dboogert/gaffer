
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
* [?] [1 VDB input] MathOp
    * Works on scalars or Vectors 
        * Weird combination of grad(S) -> V , laplacian(S) -> S, div(V) -> S, curl(V) ->V , curvature(S-L) -> S
* [LF][1 VDB input] Statistics
    * (S, V, S-L) 
    * rename attribute to statistics:[grid]:[stat]
* [L] [2 VDB inputs] CSGGrids
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

Add mask to filtering and morphology            
copy grids from one VDB into another
segment SDF LevelSet
fracture levelset
Advect points
LevelSetAdvection
OSL                                   
Ray Intersection                                                                                                                                                                                                                                                                                                                                                                                                        