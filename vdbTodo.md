
## Random Ideas:

* flatpak packaging of gaffer
* gaffer hub
* Azure pipelines for test builds
* Handle addtional grid types
* VDB Scene writer
* Discuss 2nd input scene locations 
    * It isn't obvious which paths reference which inputs

## Nodes to make:

   * Copy grid from one VDB to another 
   * MarchVolume (fog in particular)
   * AdvectGrids
   * AdvectPoints
   * FractureLevelSets
   * MorphLevelSets
   * SetBackgroundGrids
   * DiagnosticsGrids
   * PruneGrids
   * PotentialFlow
   * Render VDB to an image
   
## Nodes to create a PR for:

* SphereLevelSet (ObjectSource)
    * Tests 
    * Documentation
    * Examples
* PlatonicLevelSet (ObjectSource)
    * Tests
    * Documentation
    * Examples
* VDBObject (ObjectSource)
    * What should the bbox be set to
    * Tests
    * Documentation
    * Example
* StatisticsGrids (SceneElementProcessor)
    * Tests
    * Documentation
    * Example
* MeasureLevelSet -> MeasureLevelSets (SceneElementProcessor)
    * Tests
    * Documentation
    * Examples
* VolumeToSpheres (ObjectProcessor)
    * Check primitive transforms
    * Tests
    * Documentation
    * Example    
* ScatterPoints (ObjectProcessor)
    * Check primitive transforms
    * Tests 
    * Documentation
    * Examples
* SegmentLevelSet (ObjectProcessor)
    * Tests
    * Documentation
    * Examples
* PointsToLevelSet (Deformer)
    * Check PointPrimitive transforms
    * Tests
    * Documentation
    * Examples
* OffsetLevelSets (Deformer)
    * Tests
    * Documentation
    * Examples
    * Interrupt canceller
* MathOpGrids (ObjectProcessor)
    * Tests
    * Documentation
    * Examples
* MaskGrids (ObjectProcessor)
    * Tests
    * Documentation
    * Examples
* LevelSetToFog (ObjectProcessor)
    * Tests
    * Documentation
    * Examples
* IntersectLevelSet (Deformer)
    * Check primitive transforms
    * Tests
    * Documentation
    * Examples
* FilterLevelSet -> FilterLevelSets (ObjectProcessor)
    * Tests
    * Documentation
    * Examples
    * Add Mask input
* FilterGrids (ObjectProcessor)
    * Tests
    * Documentation
    * Examples
    * Add Mask input
* DeleteGrids (Deformer)
    * Tests
    * Documentation
    * Examples
* CSGLevelSets (Deformer)
    * Check primitive transforms
    * Tests
    * Documentation
    * Examples
* CompositeGrids (Deformer)
     * Check primitive transforms
    * Tests
    * Documentation
    * Examples
* ClipGrids (Deformer)
    * Tests
    * Documentation
    * Examples
* OSLVDB
    * Tests
    * Documentation
    * Examples


## Bugs: 

* SampleGrids can't sample non scalar ( float ) values
* IntersectLevelSet (intersection doesn't seem right)
* FilterGrids - disable doesn't pass through input vdb