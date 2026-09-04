#include "VegetatonIndexGen.h"

VegetatonIndexGen::VegetatonIndexGen()
{    
}

VegetationIndex* VegetatonIndexGen::InitVegetationIndex()
{
    VegetationIndex *idx;

    if(ndvi_type != 0)
        idx = new Ndvi(0, ndvi_type);
    else if(grvi_type != 0)
        idx = new Grvi(grvi_type);

    return idx;
}
