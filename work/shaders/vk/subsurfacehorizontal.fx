//------------------------------------------------------------------------------
//  subsurfacevertical.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#define HORIZONTAL

#include "lib/std.fxh"
#include "lib/subsurface.fxh"
#include "lib/techniques.fxh"

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), SubsurfaceState);
