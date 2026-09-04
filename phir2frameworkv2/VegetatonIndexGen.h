#ifndef VEGETATONINDEXGEN_H
#define VEGETATONINDEXGEN_H

#include "VegetationIndex.h"
#include "Ndvi.h"
#include "Grvi.h"
#include "config.h"

class VegetatonIndexGen
{
public:    
    VegetatonIndexGen();
    static VegetationIndex* InitVegetationIndex();
};

#endif // VEGETATONINDEXGEN_H
