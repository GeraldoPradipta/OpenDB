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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "db.h"
#include "dbShape.h"
#include "definBlockage.h"
#include "definComponent.h"
#include "definFill.h"
#include "definGCell.h"
#include "definNet.h"
#include "definNonDefaultRule.h"
#include "definPin.h"
#include "definPinProps.h"
#include "definPropDefs.h"
#include "definReader.h"
#include "definRegion.h"
#include "definRow.h"
#include "definSNet.h"
#include "definTracks.h"
#include "definVia.h"

namespace odb {

class DefHeader
{
 public:
  const char* _version;
  char        _hier_delimeter;
  char        _left_bus_delimeter;
  char        _right_bus_delimeter;
  const char* _design;

  DefHeader()
  {
    _version             = NULL;
    _hier_delimeter      = 0;
    _left_bus_delimeter  = 0;
    _right_bus_delimeter = 0;
    _design              = NULL;
  }

  ~DefHeader()
  {
    if (_version)
      free((void*) _version);

    if (_design)
      free((void*) _design);
  }

  static DefHeader* getDefHeader(const char* file);
};

definReader::definReader(dbDatabase* db)
{
  _db         = db;
  _block_name = NULL;

  _blockageR         = new definBlockage;
  _componentR        = new definComponent;
  _fillR             = new definFill;
  _gcellR            = new definGCell;
  _netR              = new definNet;
  _pinR              = new definPin;
  _rowR              = new definRow;
  _snetR             = new definSNet;
  _tracksR           = new definTracks;
  _viaR              = new definVia;
  _regionR           = new definRegion;
  _non_default_ruleR = new definNonDefaultRule;
  _prop_defsR        = new definPropDefs;
  _pin_propsR        = new definPinProps;

  _interfaces.push_back(_blockageR);
  _interfaces.push_back(_componentR);
  _interfaces.push_back(_fillR);
  _interfaces.push_back(_gcellR);
  _interfaces.push_back(_netR);
  _interfaces.push_back(_pinR);
  _interfaces.push_back(_rowR);
  _interfaces.push_back(_snetR);
  _interfaces.push_back(_tracksR);
  _interfaces.push_back(_viaR);
  _interfaces.push_back(_regionR);
  _interfaces.push_back(_non_default_ruleR);
  _interfaces.push_back(_prop_defsR);
  _interfaces.push_back(_pin_propsR);
  init();
}

definReader::~definReader()
{
  delete _blockageR;
  delete _componentR;
  delete _fillR;
  delete _gcellR;
  delete _netR;
  delete _pinR;
  delete _rowR;
  delete _snetR;
  delete _tracksR;
  delete _viaR;
  delete _regionR;
  delete _non_default_ruleR;
  delete _prop_defsR;
  delete _pin_propsR;

  if (_block_name)
    free((void*) _block_name);
}

int definReader::errors()
{
  int e = _errors;

  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr)
    e += (*itr)->_errors;

  return e;
}

void definReader::skipWires()
{
  _netR->skipWires();
}
void definReader::skipConnections()
{
  _netR->skipConnections();
}
void definReader::skipSpecialWires()
{
  _snetR->skipSpecialWires();
}

void definReader::skipShields()
{
  _snetR->skipShields();
}

void definReader::skipBlockWires()
{
  _snetR->skipBlockWires();
}

void definReader::skipFillWires()
{
  _snetR->skipFillWires();
}

void definReader::replaceWires()
{
  _netR->replaceWires();
  _snetR->replaceWires();
}

void definReader::namesAreDBIDs()
{
  _netR->namesAreDBIDs();
  _snetR->namesAreDBIDs();
}

void definReader::setAssemblyMode()
{
  _netR->setAssemblyMode();
}

void definReader::useBlockName(const char* name)
{
  if (_block_name)
    free((void*) _block_name);

  _block_name = strdup(name);
  assert(_block_name);
}

void definReader::init()
{
  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr)
    (*itr)->init();
  _update = false;
}

void definReader::setTech(dbTech* tech)
{
  definBase::setTech(tech);

  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr)
    (*itr)->setTech(tech);
}

void definReader::setBlock(dbBlock* block)
{
  definBase::setBlock(block);
  std::vector<definBase*>::iterator itr;
  for (itr = _interfaces.begin(); itr != _interfaces.end(); ++itr)
    (*itr)->setBlock(block);
}

// Generic handler for transfering properties from the
// Si2 DEF parser object to the OpenDB callback
template <typename DEF_TYPE, typename CALLBACK>
static void handle_props(DEF_TYPE* def_obj, CALLBACK* callback)
{
  for (int i = 0; i < def_obj->numProps(); ++i) {
    switch (def_obj->propType(i)) {
      case 'R':
        callback->property(def_obj->propName(i), def_obj->propNumber(i));
        break;
      case 'I':
        callback->property(def_obj->propName(i), (int) def_obj->propNumber(i));
        break;
      case 'S': /* fallthru */
      case 'N': /* fallthru */
      case 'Q':
        callback->property(def_obj->propName(i), def_obj->propValue(i));
        break;
    }
  }
}

int definReader::blockageCallback(defrCallbackType_e /* unused: type */,
                                  defiBlockage* blockage,
                                  defiUserData  data)
{
  definReader*   reader    = (definReader*) data;
  definBlockage* blockageR = reader->_blockageR;

  if (blockage->hasExceptpgnet()) {
    reader->error("EXCEPTPGNET on blockage is unsupported");
    return PARSE_ERROR;
  }

  if (blockage->hasMask()) {
    reader->error("MASK on blockage is unsupported");
    return PARSE_ERROR;
  }

  if (blockage->hasSoft()) {
    reader->error("SOFT on blockage is unsupported");
    return PARSE_ERROR;
  }

  if (blockage->hasPartial()) {
    reader->error("PARTIAL on blockage is unsupported");
    return PARSE_ERROR;
  }

  if (blockage->hasLayer()) {
    // routing blockage
    blockageR->blockageRoutingBegin(blockage->layerName());

    if (blockage->hasSlots()) {
      blockageR->blockageRoutingSlots();
    }

    if (blockage->hasFills()) {
      blockageR->blockageRoutingFills();
    }

    if (blockage->hasPushdown()) {
      blockageR->blockageRoutingPushdown();
    }

    if (blockage->hasSpacing()) {
      blockageR->blockageRoutingMinSpacing(blockage->minSpacing());
    }

    if (blockage->hasDesignRuleWidth()) {
      blockageR->blockageRoutingEffectiveWidth(blockage->designRuleWidth());
    }

    if (blockage->hasComponent()) {
      blockageR->blockageRoutingComponent(blockage->placementComponentName());
    }

    for (int i = 0; i < blockage->numRectangles(); ++i) {
      blockageR->blockageRoutingRect(
          blockage->xl(i), blockage->yl(i), blockage->xh(i), blockage->yh(i));
    }

    for (int i = 0; i < blockage->numPolygons(); ++i) {
      defiPoints            defPoints = blockage->getPolygon(i);
      std::vector<Point> points;
      reader->translate(defPoints, points);
      blockageR->blockageRoutingPolygon(points);
    }

    blockageR->blockageRoutingEnd();
  } else {
    // placement blockage
    blockageR->blockagePlacementBegin();

    if (blockage->hasComponent()) {
      blockageR->blockagePlacementComponent(blockage->placementComponentName());
    }

    if (blockage->hasPushdown()) {
      blockageR->blockagePlacementPushdown();
    }

    for (int i = 0; i < blockage->numRectangles(); ++i) {
      blockageR->blockagePlacementRect(
          blockage->xl(i), blockage->yl(i), blockage->xh(i), blockage->yh(i));
    }

    blockageR->blockagePlacementEnd();
  }

  return PARSE_OK;
}

int definReader::componentsCallback(defrCallbackType_e /* unused: type */,
                                    defiComponent*     comp,
                                    defiUserData       data)
{
  definReader*    reader     = (definReader*) data;
  definComponent* componentR = reader->_componentR;

  if (comp->hasEEQ()) {
    reader->error("EEQMASTER on component is unsupported");
    return PARSE_ERROR;
  }

  if (comp->maskShiftSize() > 0) {
    reader->error("MASKSHIFT on component is unsupported");
    return PARSE_ERROR;
  }

  if (comp->hasHalo() > 0) {
    reader->error("HALO on component is unsupported");
    return PARSE_ERROR;
  }

  if (comp->hasRouteHalo() > 0) {
    reader->error("ROUTEHALO on component is unsupported");
    return PARSE_ERROR;
  }

  componentR->begin(comp->id(), comp->name());
  if (comp->hasSource()) {
    componentR->source(dbSourceType(comp->source()));
  }
  if (comp->hasWeight()) {
    componentR->weight(comp->weight());
  }
  if (comp->hasRegionName()) {
    componentR->region(comp->regionName());
  }

  componentR->placement(comp->placementStatus(),
                        comp->placementX(),
                        comp->placementY(),
                        comp->placementOrient());

  handle_props(comp, componentR);

  componentR->end();

  return PARSE_OK;
}

int definReader::componentMaskShiftCallback(
                                            defrCallbackType_e           /* unused: type */,
                                            defiComponentMaskShiftLayer* /* unused: shiftLayers */,
    defiUserData                 data)
{
  definReader* reader = (definReader*) data;
  reader->error("COMPONENTMASKSHIFT is unsupported");
  return PARSE_ERROR;
}

int definReader::dieAreaCallback(defrCallbackType_e /* unused: type */,
                                 defiBox*           box,
                                 defiUserData       data)
{
  definReader* reader = (definReader*) data;

  const defiPoints points = box->getPoint();

  if (!reader->_update) {
    std::vector<Point> P;
    reader->translate(points, P);

    if (P.size() < 2) {
      notice(0, "error: Invalid DIEAREA statement, missing point(s)\n");
      ++reader->_errors;
      return PARSE_ERROR;
    }

    if (P.size() == 2) {
      Point p0 = P[0];
      Point p1 = P[1];
      Rect  r(p0.getX(), p0.getY(), p1.getX(), p1.getY());
      reader->_block->setDieArea(r);
    } else {
      notice(0,
             "warning: Polygon DIEAREA statement not supported.  The bounding "
             "box will be used instead\n");
      int                             xmin = INT_MAX;
      int                             ymin = INT_MAX;
      int                             xmax = INT_MIN;
      int                             ymax = INT_MIN;
      std::vector<Point>::iterator itr;

      for (itr = P.begin(); itr != P.end(); ++itr) {
        Point& p = *itr;
        int       x = p.getX();
        int       y = p.getY();

        if (x < xmin)
          xmin = x;

        if (y < ymin)
          ymin = y;

        if (x > xmax)
          xmax = x;

        if (y > ymax)
          ymax = y;
      }

      Rect r(xmin, ymin, xmax, ymax);
      reader->_block->setDieArea(r);
    }
  }
  return PARSE_OK;
}

int definReader::extensionCallback(defrCallbackType_e /* unused: type */,
                                   const char*        /* unused: extension */,
                                   defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->error("Syntax extensions (BEGINEXT/ENDEXT) are unsupported");
  return PARSE_ERROR;
}

int definReader::fillsCallback(defrCallbackType_e /* unused: type */,
                               int                /* unused: count */,
                               defiUserData       data)
{
  definReader* reader = (definReader*) data;

  // definFill doesn't do anything! Just error out for now
  reader->error("FILL is unsupported");
  return PARSE_ERROR;
}

// This is incomplete but won't be reached because of the
// fillsCallback
int definReader::fillCallback(defrCallbackType_e /* unused: type */,
                              defiFill*          fill,
                              defiUserData       data)
{
  definReader* reader = (definReader*) data;
  definFill*   fillR  = reader->_fillR;

  // This isn't right as we don't call begin when we don't have a
  // layer.  That can happen with via fill.  However the callback is
  // incomplete so it doesn't matter yet.
  if (fill->hasLayer()) {
    fillR->fillBegin(fill->layerName());
  }

  for (int i = 0; i < fill->numRectangles(); ++i) {
    fillR->fillRect(fill->xl(i), fill->yl(i), fill->xh(i), fill->yh(i));
  }

  for (int i = 0; i < fill->numPolygons(); ++i) {
    defiPoints            defPoints = fill->getPolygon(i);
    std::vector<Point> points;
    reader->translate(defPoints, points);

    fillR->fillPolygon(points);
  }

  fillR->fillEnd();

  return PARSE_OK;
}

int definReader::gcellGridCallback(defrCallbackType_e /* unused: type */,
                                   defiGcellGrid*     grid,
                                   defiUserData       data)
{
  definReader* reader = (definReader*) data;
  defDirection dir    = (grid->macro()[0] == 'X') ? DEF_X : DEF_Y;

  reader->_gcellR->gcell(dir, grid->x(), grid->xNum(), grid->xStep());

  return PARSE_OK;
}

int definReader::groupNameCallback(defrCallbackType_e /* unused: type */,
                                   const char*        name,
                                   defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->_regionR->begin(name, /* group */ true);
  return PARSE_OK;
}

int definReader::groupMemberCallback(defrCallbackType_e /* unused: type */,
                                     const char*        member,
                                     defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->_regionR->inst(member);
  return PARSE_OK;
}

int definReader::groupCallback(defrCallbackType_e /* unused: type */,
                               defiGroup*         group,
                               defiUserData       data)
{
  definReader* reader  = (definReader*) data;
  definRegion* regionR = reader->_regionR;

  if (group->hasRegionName()) {
    regionR->parent(group->regionName());
  }
  handle_props(group, regionR);
  regionR->end();

  return PARSE_OK;
}

int definReader::historyCallback(defrCallbackType_e /* unused: type */,
                                 const char*        /* unused: extension */,
                                 defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->error("HISTORY is unsupported");
  return PARSE_ERROR;
}

// This is a ugly workaround.  We handle precisely the case that a
// path consists of a single layer/point/rect sequence of min width
// and nothing more.  This is all TritonRoute writes to DEF currently
// so that is all we support (used for min area violations).  Anything
// else will generate a parser error.
static bool handleRectPath(defiPath* path, definNet* netR)
{
  const char* layerName    = nullptr;
  bool        hasPoint = false;
  bool        hasRect  = false;
  int         x;
  int         y;
  int         deltaX1;
  int         deltaY1;
  int         deltaX2;
  int         deltaY2;

  path->initTraverse();
  int pathId;
  while ((pathId = path->next()) != DEFIPATH_DONE) {
    switch (pathId) {
      case DEFIPATH_LAYER: {
        layerName = path->getLayer();
        break;
      }
      case DEFIPATH_POINT: {
        if (hasPoint) {
          return false;
        }
        hasPoint = true;
        path->getPoint(&x, &y);
        break;
      }
      case DEFIPATH_RECT: {
        if (hasRect) {
          return false;
        }
        hasRect = true;
        path->getViaRect(&deltaX1, &deltaY1, &deltaX2, &deltaY2);
        break;
      }
      default:
        return false;
    }
  }

  netR->path(layerName);
  int minWidth = netR->getLayer()->getWidth();
  int ext = minWidth / 2;

  if (deltaX2 - deltaX1 == minWidth) {  // vertical
    if (-deltaX1 != deltaX2) { // must be centered on this point
      return false;
    }
    netR->pathPoint(x, y + deltaY1 + ext);
    netR->pathPoint(x, y + deltaY2 - ext);
  } else if (deltaY2 - deltaY1 == minWidth) {  // horizontal
    if (-deltaY1 != deltaY2) { // must be centered on this point
      return false;
    }
    netR->pathPoint(x + deltaX1 + ext, y);
    netR->pathPoint(x + deltaX2 - ext, y);
  } else {
    return false;
  }
  netR->pathEnd();

  return true;
}

int definReader::netCallback(defrCallbackType_e /* unused: type */,
                             defiNet*           net,
                             defiUserData       data)
{
  definReader* reader = (definReader*) data;
  definNet*    netR   = reader->_netR;

  if (net->numShieldNets() > 0) {
    reader->error("SHIELDNET on net is unsupported");
    return PARSE_ERROR;
  }

  if (net->numVpins() > 0) {
    reader->error("VPIN on net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasSubnets()) {
    reader->error("SUBNET on net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasXTalk()) {
    reader->error("XTALK on net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasFrequency()) {
    reader->error("FREQUENCY on net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasOriginal()) {
    reader->error("ORIGINAL on net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasPattern()) {
    reader->error("PATTERN on net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasCap()) {
    reader->error("ESTCAP on net is unsupported");
    return PARSE_ERROR;
  }

  netR->begin(net->name());

  if (net->hasUse()) {
    netR->use(net->use());
  }

  if (net->hasSource()) {
    netR->source(net->source());
  }

  if (net->hasFixedbump()) {
    netR->fixedbump();
  }

  if (net->hasWeight()) {
    netR->weight(net->weight());
  }

  if (net->hasNonDefaultRule()) {
    netR->nonDefaultRule(net->nonDefaultRule());
  }

  for (int i = 0; i < net->numConnections(); ++i) {
    if (net->pinIsSynthesized(i)) {
      reader->error("SYNTHESIZED on net's connection is unsupported");
      return PARSE_ERROR;
    }

    if (net->pinIsMustJoin(i)) {
      netR->beginMustjoin(net->instance(i), net->pin(i));
    } else {
      netR->connection(net->instance(i), net->pin(i));
    }
  }

  for (int i = 0; i < net->numWires(); ++i) {
    defiWire* wire = net->wire(i);
    netR->wire(wire->wireType());

    for (int j = 0; j < wire->numPaths(); ++j) {
      defiPath* path = wire->path(j);

      if (handleRectPath(path, netR)) {
        continue;
      }

      path->initTraverse();

      int pathId;
      while ((pathId = path->next()) != DEFIPATH_DONE) {
        switch (pathId) {
          case DEFIPATH_LAYER: {
            // We need to peek ahead to see if there is a taper next
            const char* layer  = path->getLayer();
            int         nextId = path->next();
            if (nextId == DEFIPATH_TAPER) {
              netR->pathTaper(layer);
            } else if (nextId == DEFIPATH_TAPERRULE) {
              netR->pathTaperRule(layer, path->getTaperRule());
            } else {
              netR->path(layer);
              path->prev();  // put back the token
            }
            break;
          }

          case DEFIPATH_VIA: {
            // We need to peek ahead to see if there is a rotation next
            const char* viaName = path->getVia();
            int         nextId  = path->next();
            if (nextId == DEFIPATH_VIAROTATION) {
              netR->pathVia(viaName,
                            translate_orientation(path->getViaRotation()));
            } else {
              netR->pathVia(viaName);
              path->prev();  // put back the token
            }
            break;
          }

          case DEFIPATH_POINT: {
            int x;
            int y;
            path->getPoint(&x, &y);
            netR->pathPoint(x, y);
            break;
          }

          case DEFIPATH_FLUSHPOINT: {
            int x;
            int y;
            int ext;
            path->getFlushPoint(&x, &y, &ext);
            netR->pathPoint(x, y, ext);
            break;
          }

          case DEFIPATH_STYLE:
            netR->pathStyle(path->getStyle());
            return PARSE_ERROR;  // callback issues error
            break;

          case DEFIPATH_RECT: {
            return PARSE_ERROR;
            break;
          }

          case DEFIPATH_VIRTUALPOINT:
            reader->error("VIRTUAL in net's routing is unsupported");
            return PARSE_ERROR;

          case DEFIPATH_MASK:
          case DEFIPATH_VIAMASK:
            reader->error("MASK in net's routing is unsupported");
            return PARSE_ERROR;

          default:
            reader->error("Unknown construct in net's routing is unsupported");
            return PARSE_ERROR;
        }
      }
      netR->pathEnd();
    }

    netR->wireEnd();
  }

  handle_props(net, netR);

  netR->end();

  return PARSE_OK;
}

int definReader::nonDefaultRuleCallback(defrCallbackType_e /* unused: type */,
                                        defiNonDefault*    rule,
                                        defiUserData       data)
{
  definReader*         reader = (definReader*) data;
  definNonDefaultRule* ruleR  = reader->_non_default_ruleR;

  ruleR->beginRule(rule->name());

  if (rule->hasHardspacing()) {
    ruleR->hardSpacing();
  }

  for (int i = 0; i < rule->numLayers(); ++i) {
    if (rule->hasLayerDiagWidth(i)) {
      reader->error("DIAGWIDTH on non-default rule is unsupported");
      return PARSE_ERROR;
    }

    ruleR->beginLayerRule(rule->layerName(i), rule->layerWidthVal(i));

    if (rule->hasLayerSpacing(i)) {
      ruleR->spacing(rule->layerSpacingVal(i));
    }

    if (rule->hasLayerWireExt(i)) {
      ruleR->wireExt(rule->layerWireExtVal(i));
    }

    ruleR->endLayerRule();
  }

  for (int i = 0; i < rule->numVias(); ++i) {
    ruleR->via(rule->viaName(i));
  }

  for (int i = 0; i < rule->numViaRules(); ++i) {
    ruleR->viaRule(rule->viaRuleName(i));
  }

  for (int i = 0; i < rule->numMinCuts(); ++i) {
    ruleR->minCuts(rule->cutLayerName(i), rule->numCuts(i));
  }

  handle_props(rule, ruleR);

  ruleR->endRule();

  return PARSE_OK;
}

int definReader::pinCallback(defrCallbackType_e /* unused: type */,
                             defiPin*           pin,
                             defiUserData       data)
{
  definReader* reader = (definReader*) data;
  definPin*    pinR   = reader->_pinR;

  if (pin->numVias() > 0) {
    reader->error("VIA in pins is unsupported");
    return PARSE_ERROR;
  }

  if (pin->hasNetExpr()) {
    reader->error("NETEXPR on pin is unsupported");
    return PARSE_ERROR;
  }

  if (pin->hasAPinPartialMetalArea() || pin->hasAPinPartialMetalSideArea()
      || pin->hasAPinDiffArea() || pin->hasAPinPartialCutArea()
      || pin->numAntennaModel() > 0) {
    reader->error("Antenna data on pin is unsupported");
    return PARSE_ERROR;
  }

  if (pin->numPorts() > 1) {
    // TritonRoute doesn't support these either
    notice(0, "error: pin with multiple ports is not supported\n");
    ++reader->_errors;
    return PARSE_ERROR;
  }

  if (pin->numPolygons() > 0) {
    // The db does support polygons but the callback code seems incorrect to me
    // (ignores layers!).  Delaying support until I can fix it.
    notice(0, "error: polygons in pins are not supported\n");
    ++reader->_errors;
    return PARSE_ERROR;
  }

  pinR->pinBegin(pin->pinName(), pin->netName());

  if (pin->hasSpecial()) {
    pinR->pinSpecial();
  }

  if (pin->hasUse()) {
    pinR->pinUse(pin->use());
  }

  if (pin->hasDirection()) {
    pinR->pinDirection(pin->direction());
  }

  if (pin->hasSupplySensitivity()) {
    pinR->pinSupplyPin(pin->supplySensitivity());
  }

  if (pin->hasGroundSensitivity()) {
    pinR->pinGroundPin(pin->groundSensitivity());
  }

  for (int i = 0; i < pin->numLayer(); ++i) {
    if (pin->layerMask(i) != 0) {
      reader->error("MASK on pin's layer is unsupported");
      return PARSE_ERROR;
    }

    int xl;
    int yl;
    int xh;
    int yh;
    pin->bounds(i, &xl, &yl, &xh, &yh);
    pinR->pinRect(pin->layer(i), xl, yl, xh, yh);

    if (pin->hasLayerSpacing(i)) {
      pinR->pinMinSpacing(pin->layerSpacing(i));
    }

    if (pin->hasLayerDesignRuleWidth(i)) {
      pinR->pinEffectiveWidth(pin->layerDesignRuleWidth(i));
    }
  }

  if (pin->hasPlacement()) {
    defPlacement type = DEF_PLACEMENT_UNPLACED;
    if (pin->isPlaced()) {
      type = DEF_PLACEMENT_PLACED;
    } else if (pin->isCover()) {
      type = DEF_PLACEMENT_COVER;
    } else if (pin->isFixed()) {
      type = DEF_PLACEMENT_FIXED;
    } else {
      assert(0);
    }
    dbOrientType orient = reader->translate_orientation(pin->orient());
    pinR->pinPlacement(type, pin->placementX(), pin->placementY(), orient);
  }

  pinR->pinEnd();

  return PARSE_OK;
}

int definReader::pinsEndCallback(defrCallbackType_e /* unused: type */,
                                 void*              /* unused: v */,
                                 defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->_pinR->pinsEnd();
  return PARSE_OK;
}

int definReader::pinPropCallback(defrCallbackType_e /* unused: type */,
                                 defiPinProp*       prop,
                                 defiUserData       data)
{
  definReader*   reader = (definReader*) data;
  definPinProps* propR  = reader->_pin_propsR;

  propR->begin(prop->isPin() ? "PIN" : prop->instName(), prop->pinName());
  handle_props(prop, propR);
  propR->end();

  return PARSE_OK;
}

int definReader::pinsStartCallback(defrCallbackType_e /* unused: type */,
                                   int                number,
                                   defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->_pinR->pinsBegin(number);
  return PARSE_OK;
}

int definReader::propCallback(defrCallbackType_e /* unused: type */,
                              defiProp*          prop,
                              defiUserData       data)
{
  definReader*   reader     = (definReader*) data;
  definPropDefs* prop_defsR = reader->_prop_defsR;

  defPropType data_type;
  switch (prop->dataType()) {
    case 'I':
      data_type = DEF_INTEGER;
      break;
    case 'R':
      data_type = DEF_REAL;
      break;
    case 'S': /* fallthru */
    case 'N': /* fallthru */
    case 'Q':
      data_type = DEF_STRING;
      break;
    default:
      assert(0);
      return PARSE_ERROR;
  }

  // The prop type should be upper case for consistency
  std::string prop_type(prop->propType());
  for (auto& c : prop_type) {
    c = toupper(c);
  }

  prop_defsR->begin(prop_type.c_str(), prop->propName(), data_type);

  if (prop->hasRange()) {
    if (data_type == DEF_INTEGER) {
      // Call the integer overload
      prop_defsR->range((int) prop->left(), (int) prop->right());
    } else {
      assert(data_type == DEF_REAL);
      // Call the double overload
      prop_defsR->range(prop->left(), prop->right());
    }
  }

  switch (data_type) {
    case DEF_INTEGER:
      if (prop->hasNumber()) {
        prop_defsR->value((int) prop->number());  // int overload
      }
      break;
    case DEF_REAL:
      if (prop->hasNumber()) {
        prop_defsR->value(prop->number());  // double overload
      }
      break;
    case DEF_STRING:
      if (prop->hasString()) {
        prop_defsR->value(prop->string());  // string overload
      }
      break;
  }

  prop_defsR->end();

  return PARSE_OK;
}

int definReader::propEndCallback(defrCallbackType_e /* unused: type */,
                                 void*              /* unused: v */,
                                 defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->_prop_defsR->endDefinitions();
  return PARSE_OK;
}

int definReader::propStartCallback(defrCallbackType_e /* unused: type */,
                                   void*              /* unused: v */,
                                   defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->_prop_defsR->beginDefinitions();
  return PARSE_OK;
}

int definReader::regionCallback(defrCallbackType_e /* unused: type */,
                                defiRegion*        region,
                                defiUserData       data)
{
  definReader* reader  = (definReader*) data;
  definRegion* regionR = reader->_regionR;

  regionR->begin(region->name(), /* is_group */ false);

  for (int i = 0; i < region->numRectangles(); ++i) {
    regionR->boundary(
        region->xl(i), region->yl(i), region->xh(i), region->yh(i));
  }

  if (region->hasType()) {
    const char* type = region->type();
    if (strcmp(type, "FENCE") == 0) {
      regionR->type(DEF_FENCE);
    } else {
      assert(strcmp(type, "GUIDE") == 0);
      regionR->type(DEF_GUIDE);
    }
  }

  handle_props(region, regionR);

  regionR->end();

  return PARSE_OK;
}

int definReader::rowCallback(defrCallbackType_e /* unused: type */,
                             defiRow*           row,
                             defiUserData       data)
{
  definReader* reader = (definReader*) data;
  definRow*    rowR   = reader->_rowR;

  defRow dir       = DEF_HORIZONTAL;
  int    num_sites = 1;
  int    spacing   = 0;

  if (row->hasDo()) {
    if (row->yNum() == 1) {
      dir       = DEF_HORIZONTAL;
      num_sites = row->xNum();
      if (row->hasDoStep()) {
        spacing = row->xStep();
      }
    } else {
      dir       = DEF_VERTICAL;
      num_sites = row->yNum();
      if (row->hasDoStep()) {
        spacing = row->yStep();
      }
    }
  }

  rowR->begin(row->name(),
              row->macro(),
              row->x(),
              row->y(),
              reader->translate_orientation(row->orient()),
              dir,
              num_sites,
              spacing);

  handle_props(row, rowR);

  reader->_rowR->end();

  return PARSE_OK;
}

int definReader::scanchainsCallback(defrCallbackType_e /* unused: type */,
                                    int                /* unused: count */,
                                    defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->error("SCANCHAINS are unsupported");
  return PARSE_ERROR;
}

int definReader::slotsCallback(defrCallbackType_e /* unused: type */,
                               int                /* unused: count */,
                               defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->error("SLOTS are unsupported");
  return PARSE_ERROR;
}

int definReader::stylesCallback(defrCallbackType_e /* unused: type */,
                                int                /* unused: count */,
                                defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->error("STYLES are unsupported");
  return PARSE_ERROR;
}

int definReader::technologyCallback(defrCallbackType_e /* unused: type */,
                                    const char*        /* unused: name */,
                                    defiUserData       data)
{
  definReader* reader = (definReader*) data;
  reader->error("TECHNOLOGY is unsupported");
  return PARSE_ERROR;
}

int definReader::trackCallback(defrCallbackType_e /* unused: type */,
                               defiTrack*         track,
                               defiUserData       data)
{
  definReader* reader = (definReader*) data;

  if (track->firstTrackMask() != 0) {
    reader->error("MASK on track is unsupported");
    return PARSE_ERROR;
  }

  defDirection dir = track->macro()[0] == 'X' ? DEF_X : DEF_Y;
  reader->_tracksR->tracksBegin(dir, track->x(), track->xNum(), track->xStep());

  for (int i = 0; i < track->numLayers(); ++i) {
    reader->_tracksR->tracksLayer(track->layer(i));
  }

  reader->_tracksR->tracksEnd();
  return PARSE_OK;
}

int definReader::unitsCallback(defrCallbackType_e, double d, defiUserData data)
{
  definReader* reader = (definReader*) data;

  // Truncation error
  if (d > reader->_tech->getDbUnitsPerMicron()) {
    notice(0,
           "error: The DEF UNITS DISTANCE MICRONS convert factor (%d) is "
           "greater than the database units per micron (%d) value.\n",
           (int) d,
           reader->_tech->getDbUnitsPerMicron());
    ++reader->_errors;
    return PARSE_ERROR;
  }

  reader->units(d);

  std::vector<definBase*>::iterator itr;
  for (itr = reader->_interfaces.begin(); itr != reader->_interfaces.end();
       ++itr)
    (*itr)->units(d);

  if (!reader->_update)
    reader->_block->setDefUnits(d);
  return PARSE_OK;
}

int definReader::viaCallback(defrCallbackType_e /* unused: type */,
                             defiVia*           via,
                             defiUserData       data)
{
  definReader* reader = (definReader*) data;
  definVia*    viaR   = reader->_viaR;

  if (via->numPolygons() > 0) {
    reader->error("POLYGON in via is unsupported");
    return PARSE_ERROR;
  }

  viaR->viaBegin(via->name());

  if (via->hasViaRule()) {
    char* viaRuleName;
    int   xSize;
    int   ySize;
    char* botLayer;
    char* cutLayer;
    char* topLayer;
    int   xCutSpacing;
    int   yCutSpacing;
    int   xBotEnc;
    int   yBotEnc;
    int   xTopEnc;
    int   yTopEnc;
    via->viaRule(&viaRuleName,
                 &xSize,
                 &ySize,
                 &botLayer,
                 &cutLayer,
                 &topLayer,
                 &xCutSpacing,
                 &yCutSpacing,
                 &xBotEnc,
                 &yBotEnc,
                 &xTopEnc,
                 &yTopEnc);
    viaR->viaRule(viaRuleName);
    viaR->viaCutSize(xSize, ySize);
    if (!viaR->viaLayers(botLayer, cutLayer, topLayer)) {
      return PARSE_ERROR;
    }
    viaR->viaCutSpacing(xCutSpacing, yCutSpacing);
    viaR->viaEnclosure(xBotEnc, yBotEnc, xTopEnc, yTopEnc);

    if (via->hasRowCol()) {
      int numCutRows;
      int numCutCols;
      via->rowCol(&numCutRows, &numCutCols);
      viaR->viaRowCol(numCutRows, numCutCols);
    }

    if (via->hasOrigin()) {
      int xOffset;
      int yOffset;
      via->origin(&xOffset, &yOffset);
      viaR->viaOrigin(xOffset, yOffset);
    }

    if (via->hasOffset()) {
      int xBotOffset;
      int yBotOffset;
      int xTopOffset;
      int yTopOffset;
      via->offset(&xBotOffset, &yBotOffset, &xTopOffset, &yTopOffset);
      viaR->viaOffset(xBotOffset, yBotOffset, xTopOffset, yTopOffset);
    }

    if (via->hasCutPattern()) {
      viaR->viaPattern(via->cutPattern());
    }
  }

  for (int i = 0; i < via->numLayers(); ++i) {
    if (via->hasRectMask(i)) {
      reader->error("MASK on via rect is unsupported");
      return PARSE_ERROR;
    }

    char* layer;
    int   xl;
    int   yl;
    int   xh;
    int   yh;
    via->layer(i, &layer, &xl, &yl, &xh, &yh);
    viaR->viaRect(layer, xl, yl, xh, yh);
  }

  viaR->viaEnd();

  return PARSE_OK;
}

int definReader::specialNetCallback(defrCallbackType_e /* unused: type */,
                                    defiNet*           net,
                                    defiUserData       data)
{
  definReader* reader = (definReader*) data;
  definSNet*   snetR  = reader->_snetR;

  if (net->hasCap()) {
    reader->error("ESTCAP on special net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasPattern()) {
    reader->error("PATTERN on special net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasOriginal()) {
    reader->error("ORIGINAL on special net is unsupported");
    return PARSE_ERROR;
  }

  if (net->numShieldNets() > 0) {
    reader->error("SHIELDNET on special net is unsupported");
    return PARSE_ERROR;
  }

  if (net->hasVoltage()) {
    reader->error("VOLTAGE on special net is unsupported");
    return PARSE_ERROR;
  }

  if (net->numPolygons() > 0) {
    // The db does support polygons but the callback code seems incorrect to me
    // (ignores layers!).  Delaying support until I can fix it.
    notice(0, "error: polygons in special nets are not supported\n");
    ++reader->_errors;
    return PARSE_ERROR;
  }

  if (net->numViaSpecs() > 0) {
    reader->error("VIA in special net is unsupported");
    return PARSE_ERROR;
  }

  snetR->begin(net->name());

  if (net->hasUse()) {
    snetR->use(net->use());
  }

  if (net->hasSource()) {
    snetR->source(net->source());
  }

  if (net->hasFixedbump()) {
    snetR->fixedbump();
  }

  if (net->hasWeight()) {
    snetR->weight(net->weight());
  }

  for (int i = 0; i < net->numConnections(); ++i) {
    snetR->connection(net->instance(i), net->pin(i), net->pinIsSynthesized(i));
  }

  if (net->numRectangles()) {
    for (int i = 0; i < net->numRectangles(); i++) {
      snetR->wire(net->rectShapeType(i), net->rectRouteStatusShieldName(i));
      snetR->rect(
          net->rectName(i), net->xl(i), net->yl(i), net->xh(i), net->yh(i));
      snetR->wireEnd();
    }
  }

  for (int i = 0; i < net->numWires(); ++i) {
    defiWire* wire = net->wire(i);
    snetR->wire(wire->wireType(), wire->wireShieldNetName());

    for (int j = 0; j < wire->numPaths(); ++j) {
      defiPath* path = wire->path(j);

      path->initTraverse();

      std::string layerName;

      int pathId;
      while ((pathId = path->next()) != DEFIPATH_DONE) {
        switch (pathId) {
          case DEFIPATH_LAYER:
            layerName = path->getLayer();
            break;

          case DEFIPATH_VIA: {
            // We need to peek ahead to see if there is a rotation next
            const char* viaName = path->getVia();
            int         nextId  = path->next();
            if (nextId == DEFIPATH_VIAROTATION) {
              reader->error("Rotated via in special net is unsupported");
              return PARSE_ERROR;
              // TODO: Make this take and store rotation
              // snetR->pathVia(viaName,
              //                translate_orientation(path->getViaRotation()));
            } else {
              snetR->pathVia(viaName);
              path->prev();  // put back the token
            }
            break;
          }

          case DEFIPATH_WIDTH:
            assert(!layerName.empty());  // always "layerName routeWidth"
            snetR->path(layerName.c_str(), path->getWidth());
            break;

          case DEFIPATH_POINT: {
            int x;
            int y;
            path->getPoint(&x, &y);
            snetR->pathPoint(x, y);
            break;
          }

          case DEFIPATH_FLUSHPOINT: {
            int x;
            int y;
            int ext;
            path->getFlushPoint(&x, &y, &ext);
            snetR->pathPoint(x, y, ext);
            break;
          }

          case DEFIPATH_SHAPE:
            snetR->pathShape(path->getShape());
            break;

          case DEFIPATH_STYLE:
            snetR->pathStyle(path->getStyle());
            return PARSE_ERROR;  // callback issues error
            break;

          case DEFIPATH_MASK:
          case DEFIPATH_VIAMASK:
            reader->error("MASK in special net's routing is unsupported");
            return PARSE_ERROR;

          default:
            reader->error(
                "Unknown construct in special net's routing is unsupported");
            return PARSE_ERROR;
        }
      }
      snetR->pathEnd();
    }

    snetR->wireEnd();
  }

  handle_props(net, snetR);

  snetR->end();

  return PARSE_OK;
}

void definReader::line(int line_num)
{
  notice(0, "lines processed: %d\n", line_num);
}

void definReader::error(const char* msg)
{
  notice(0, "error: %s\n", msg);
  ++_errors;
}

void definReader::setLibs(std::vector<dbLib*>& libs)
{
  _componentR->setLibs(libs);
  _rowR->setLibs(libs);
}

DefHeader* DefHeader::getDefHeader(const char* file)
{
  FILE* f = fopen(file, "r");

  if (f == NULL) {
    fprintf(stderr, "Cannot open DEF file %s\n", file);
    return NULL;
  }

  int        l;
  char       line[8192];
  DefHeader* hdr = new DefHeader();

  for (l = 1; fgets(line, 8192, f); ++l) {
    const char* token = strtok(line, " \t\n");

    if (token == NULL)
      continue;

    if ((strcmp(token, "VERSION") == 0) || (strcmp(token, "version") == 0)) {
      const char* version = strtok(NULL, " \t\n");

      if (version == NULL) {
        fprintf(stderr, "Error: Cannot read VERSION statment at line %d\n", l);
        delete hdr;
        fclose(f);
        return NULL;
      }
      hdr->_version = strdup(version);
      assert(hdr->_version);
      continue;
    }

    if ((strcmp(token, "DESIGN") == 0) || (strcmp(token, "design") == 0)) {
      const char* design = strtok(NULL, " \t\n");

      if (design == NULL) {
        fprintf(stderr, "Error: Cannot read DESIGN statment at line %d\n", l);
        delete hdr;
        fclose(f);
        return NULL;
      }

      hdr->_design = strdup(design);
      assert(hdr->_design);
      break;
    }

    if ((strcmp(token, "DIVIDERCHAR") == 0)
        || (strcmp(token, "dividerchar") == 0)) {
      const char* divider = strtok(NULL, " \t\"");

      if (divider == NULL) {
        fprintf(
            stderr, "Error: Cannot read DIVIDERCHAR statment at line %d\n", l);
        delete hdr;
        fclose(f);
        return NULL;
      }

      hdr->_hier_delimeter = divider[0];

      if (hdr->_hier_delimeter == 0) {
        fprintf(stderr,
                "Error: Syntax error in DIVIDERCHAR statment at line %d\n",
                l);
        delete hdr;
        fclose(f);
        return NULL;
      }
      continue;
    }

    if ((strcmp(token, "BUSBITCHARS") == 0)
        || (strcmp(token, "busbitchars") == 0)) {
      const char* busbitchars = strtok(NULL, " \t\"");

      if (busbitchars == NULL) {
        fprintf(
            stderr, "Error: Cannot read BUSBITCHARS statment at line %d\n", l);
        delete hdr;
        fclose(f);
        return NULL;
      }

      hdr->_left_bus_delimeter  = busbitchars[0];
      hdr->_right_bus_delimeter = busbitchars[1];

      if ((hdr->_left_bus_delimeter == 0) || (hdr->_right_bus_delimeter == 0)) {
        fprintf(stderr,
                "Error: Syntax error in BUSBITCHARS statment at line %d\n",
                l);
        delete hdr;
        fclose(f);
        return NULL;
      }

      continue;
    }

    if ((strcmp(token, "COMPONENTS") == 0)
        || (strcmp(token, "components") == 0)) {
      fprintf(stderr, "Error: DESIGN statement is missing\n");
      delete hdr;
      fclose(f);
      return NULL;
    }
  }

  fclose(f);
  return hdr;
}

dbChip* definReader::createChip(std::vector<dbLib*>& libs, const char* file)
{
  init();
  setLibs(libs);

  DefHeader* hdr = DefHeader::getDefHeader(file);

  if (hdr == NULL)
    return NULL;

  if (_db->getChip()) {
    fprintf(stderr, "Error: Chip already exists\n");
    return NULL;
  }

  dbChip* chip = dbChip::create(_db);
  assert(chip);

  if (_block_name)
    _block = dbBlock::create(chip, _block_name, hdr->_hier_delimeter);
  else
    _block = dbBlock::create(chip, hdr->_design, hdr->_hier_delimeter);

  assert(_block);
  setBlock(_block);
  setTech(_db->getTech());

  _block->setBusDelimeters(hdr->_left_bus_delimeter, hdr->_right_bus_delimeter);

  notice(0, "\nReading DEF file: %s\n", file);
  notice(0, "Design: %s\n", hdr->_design);

  if (!createBlock(file)) {
    delete hdr;
    dbChip::destroy(chip);
    notice(0, "Error: Failed to read DEF file\n");
    return NULL;
  }

  if (_pinR->_bterm_cnt)
    notice(0, "    Created %d pins.\n", _pinR->_bterm_cnt);

  if (_componentR->_inst_cnt)
    notice(0,
           "    Created %d components and %d component-terminals.\n",
           _componentR->_inst_cnt,
           _componentR->_iterm_cnt);

  if (_snetR->_snet_cnt)
    notice(0,
           "    Created %d special nets and %d connections.\n",
           _snetR->_snet_cnt,
           _snetR->_snet_iterm_cnt);

  if (_netR->_net_cnt)
    notice(0,
           "    Created %d nets and %d connections.\n",
           _netR->_net_cnt,
           _netR->_net_iterm_cnt);

  notice(0, "Finished DEF file: %s\n", file);
  delete hdr;
  return chip;
}

static std::string renameBlock(dbBlock* parent, const char* old_name)
{
  int cnt = 1;

  for (;; ++cnt) {
    char n[16];
    snprintf(n, 15, "_%d", cnt);
    std::string name(old_name);
    name += n;

    if (!parent->findChild(name.c_str()))
      return name;
  }
}

dbBlock* definReader::createBlock(dbBlock*             parent,
                                  std::vector<dbLib*>& libs,
                                  const char*          def_file)
{
  init();
  setLibs(libs);

  DefHeader* hdr = DefHeader::getDefHeader(def_file);

  if (hdr == NULL) {
    fprintf(stderr, "Header information missing from DEF file.\n");
    return NULL;
  }

  std::string block_name;

  if (_block_name)
    block_name = _block_name;
  else
    block_name = hdr->_design;

  if (parent->findChild(block_name.c_str())) {
    std::string new_name = renameBlock(parent, block_name.c_str());
    fprintf(stderr,
            "Warning: Block with name \"%s\" already exists, renaming too "
            "\"%s\".\n",
            block_name.c_str(),
            new_name.c_str());
    block_name = new_name;
  }

  _block = dbBlock::create(parent, block_name.c_str(), hdr->_hier_delimeter);

  if (_block == NULL) {
    fprintf(stderr,
            "Error: Failed to create Block with name \"%s\".\n",
            block_name.c_str());
    delete hdr;
    return NULL;
  }

  setBlock(_block);
  setTech(_db->getTech());

  _block->setBusDelimeters(hdr->_left_bus_delimeter, hdr->_right_bus_delimeter);

  notice(0, "\nReading DEF file: %s\n", def_file);
  notice(0, "Design: %s\n", hdr->_design);

  if (!createBlock(def_file)) {
    dbBlock::destroy(_block);
    notice(0, "Error: Failed to read DEF file\n");
    delete hdr;
    return NULL;
  }

  if (_pinR->_bterm_cnt)
    notice(0, "    Created %d pins.\n", _pinR->_bterm_cnt);

  if (_componentR->_inst_cnt)
    notice(0,
           "    Created %d components and %d component-terminals.\n",
           _componentR->_inst_cnt,
           _componentR->_iterm_cnt);

  if (_snetR->_snet_cnt)
    notice(0,
           "    Created %d special nets and %d connections.\n",
           _snetR->_snet_cnt,
           _snetR->_snet_iterm_cnt);

  if (_netR->_net_cnt)
    notice(0,
           "    Created %d nets and %d connections.\n",
           _netR->_net_cnt,
           _netR->_net_iterm_cnt);

  notice(0, "Finished DEF file: %s\n", def_file);

  delete hdr;
  return _block;
}

bool definReader::replaceWires(dbBlock* block, const char* def_file)
{
  init();
  setBlock(block);
  setTech(_db->getTech());

  notice(0, "\nReading DEF file: %s\n", def_file);

  if (!replaceWires(def_file)) {
    // dbBlock::destroy(_block);
    notice(0, "Error: Failed to read DEF file\n");
    return false;
  }

  if (_snetR->_snet_cnt)
    notice(0, "    Processed %d special nets.\n", _snetR->_snet_cnt);

  if (_netR->_net_cnt)
    notice(0, "    Processed %d nets.\n", _netR->_net_cnt);

  notice(0, "Finished DEF file: %s\n", def_file);
  return errors() == 0;
}

bool definReader::createBlock(const char* file)
{
  FILE* f = fopen(file, "r");

  if (f == NULL) {
    notice(0, "error: Cannot open DEF file %s\n", file);
    return false;
  }

  defrInit();
  defrReset();

  defrInitSession();

  defrSetPropCbk(propCallback);
  defrSetPropDefEndCbk(propEndCallback);
  defrSetPropDefStartCbk(propStartCallback);
  defrSetBlockageCbk(blockageCallback);
  defrSetComponentCbk(componentsCallback);
  defrSetComponentMaskShiftLayerCbk(componentMaskShiftCallback);
  defrSetDieAreaCbk(dieAreaCallback);
  defrSetExtensionCbk(extensionCallback);
  defrSetFillStartCbk(fillsCallback);
  defrSetFillCbk(fillCallback);
  defrSetGcellGridCbk(gcellGridCallback);
  defrSetGroupCbk(groupCallback);
  defrSetGroupMemberCbk(groupMemberCallback);
  defrSetGroupNameCbk(groupNameCallback);
  defrSetHistoryCbk(historyCallback);
  defrSetNetCbk(netCallback);
  defrSetNonDefaultCbk(nonDefaultRuleCallback);
  defrSetPinCbk(pinCallback);
  defrSetPinEndCbk(pinsEndCallback);
  defrSetPinPropCbk(pinPropCallback);
  defrSetRegionCbk(regionCallback);
  defrSetRowCbk(rowCallback);
  defrSetScanchainsStartCbk(scanchainsCallback);
  defrSetSlotStartCbk(slotsCallback);
  defrSetSNetCbk(specialNetCallback);
  defrSetStartPinsCbk(pinsStartCallback);
  defrSetStylesStartCbk(stylesCallback);
  defrSetTechnologyCbk(technologyCallback);
  defrSetTrackCbk(trackCallback);
  defrSetUnitsCbk(unitsCallback);
  defrSetViaCbk(viaCallback);

  defrSetAddPathToNet();

  int res = defrRead(f, file, (defiUserData) this, /* case sensitive */ 1);
  if (res != 0 || _errors != 0) {
    notice(0, "DEF parser returns an error!");
    exit(2);
  }

  defrClear();

  return true;
  // 1220 return errors() == 0;
}

bool definReader::replaceWires(const char* file)
{
  FILE* f = fopen(file, "r");

  if (f == NULL) {
    notice(0, "error: Cannot open DEF file %s\n", file);
    return false;
  }

  replaceWires();

  defrInit();
  defrReset();

  defrInitSession();

  defrSetNetCbk(netCallback);
  defrSetSNetCbk(specialNetCallback);

  defrSetAddPathToNet();

  int res = defrRead(f, file, (defiUserData) this, /* case sensitive */ 1);
  if (res != 0) {
    notice(0, "DEF parser returns an error!");
    exit(2);
  }

  defrClear();

  return true;
}

}  // namespace odb