///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef ADS_DEFIN_PROP_DEFS_H
#define ADS_DEFIN_PROP_DEFS_H

#ifndef ADS_DEFIN_IPROP_DEFS_H
#include "definIPropDefs.h"
#endif

#ifndef ADS_DEFIN_BASE_H
#include "definBase.h"
#endif

#include <string>

namespace odb {

/**********************************************************
 *
 * DEF PROPERTY DEFINITIONS are stored as hierachical
 * properties using the following structure:
 *
 *                       [__ADS_DEF_PROPERTY_DEFINITIONS__]
 *                                    +
 *                                    |
 *                                    |
 *                           +--------+------+  
 *                      [COMPONENT] [NET] [GROUP] .... 
 *                           +        |      |
 *                           |       ...    ...
 *                           |
 *                    +------+
 *                 [NAME] (type encoded as property-type)
 *                    +
 *                    |
 *                +---+--+-----+
 *             [VALUE] [MIN] [MAX] (Optional properties)
 *
 **********************************************************/

class dbProperty;

class definPropDefs : public definIPropDefs, public definBase
{
    dbProperty * _defs;
    dbProperty * _prop;
    
  public:
    virtual ~definPropDefs() {}
    virtual void beginDefinitions();
    virtual void begin( defObjectType, const char *, defPropType );
    virtual void value( const char * );
    virtual void value( int );
    virtual void value( double );
    virtual void range( int, int );
    virtual void range( double, double );
    virtual void end();
    virtual void endDefinitions();
};

} // namespace
    
#endif
