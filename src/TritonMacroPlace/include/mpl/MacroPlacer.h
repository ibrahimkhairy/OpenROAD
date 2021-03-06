///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <vector>
#include <map>
#include <set>

#include "mpl/Partition.h"

#include "opendb/db.h"

#include "sta/NetworkClass.hh"
#include "sta/GraphClass.hh"

namespace sta {
class dbSta;
class BfsFwdIterator;
class dbNetwork;
class LibertyPort;
}

namespace odb {
class dbDatabase;
class dbBTerm;
}

namespace utl {
class Logger;
}

namespace mpl {

class Layout;

typedef std::set<Macro*> MacroSet;
// vertex -> fanin macro set
typedef std::map<sta::Vertex*, MacroSet> VertexFaninMap;
typedef std::pair<Macro*, Macro*> MacroPair;
// from/to -> weight
// weight = from/pin -> to/pin count
typedef std::map<MacroPair, int> AdjWeightMap;

enum class CoreEdge
{
  West,
  East,
  North,
  South,
};

constexpr int core_edge_count = 4;
const char *coreEdgeString(CoreEdge edge);
CoreEdge coreEdgeFromIndex(int edge_index);
int coreEdgeIndex(CoreEdge edge);

class Macro
{
 public:
  double lx, ly;
  double w, h;
  double haloX, haloY;
  double channelX, channelY;
  odb::dbInst* dbInstPtr;
  Macro(double _lx,
        double _ly,
        double _w,
        double _h,
        double _haloX,
        double _haloY,
        double _channelX,
        double _channelY,
        odb::dbInst* _dbInstPtr);
  std::string name();
  std::string type();
};

class MacroLocalInfo
{
 public:
  MacroLocalInfo();
  void putHaloX(double haloX) { haloX_ = haloX; }
  void putHaloY(double haloY) { haloY_ = haloY; }
  void putChannelX(double channelX) { channelX_ = channelX; }
  void putChannelY(double channelY) { channelY_ = channelY; }
  double GetHaloX() { return haloX_; }
  double GetHaloY() { return haloY_; }
  double GetChannelX() { return channelX_; }
  double GetChannelY() { return channelY_; }

 private:
  double haloX_, haloY_, channelX_, channelY_;
};

class MacroPlacer
{
public:
  MacroPlacer();
  void init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log);

  void setHalo(double halo_x, double halo_y);
  void setChannel(double channel_x, double channel_y);
  void setVerboseLevel(int verbose);
  void setFenceRegion(double lx, double ly, double ux, double uy);

  void setGlobalConfig(const char* globalConfig);
  void setLocalConfig(const char* localConfig);

  void placeMacros();
  int getSolutionCount();

  // return weighted wire-length to get best solution
  double GetWeightedWL();
  void UpdateNetlist(Partition& layout);
  int weight(int idx11, int idx12);

private:
  // parsing function
  void ParseGlobalConfig(std::string fileName);
  void ParseLocalConfig(std::string fileName);
  void FillMacroStor();
  bool isMissingLiberty();

  void init();
  // Update Macro Location from Partition info
  void UpdateMacroCoordi(Partition& part);
  void UpdateOpendbCoordi();
  void UpdateMacroPartMap(Partition& part, MacroPartMap &macroPartMap);

  // graph based adjacencies
  void findAdjacencies();
  void seedFaninBfs(sta::BfsFwdIterator &bfs,
                    VertexFaninMap &vertex_fanins);
  void findFanins(sta::BfsFwdIterator &bfs,
                  VertexFaninMap &vertex_fanins);
  void copyFaninsAcrossRegisters(sta::BfsFwdIterator &bfs,
                                 VertexFaninMap &vertex_fanins);
  void findAdjWeights(VertexFaninMap &vertex_fanins,
                      AdjWeightMap &adj_map);
  sta::Pin *findSeqOutPin(sta::Instance *inst,
                          sta::LibertyPort *out_port);
  void fillMacroWeights(AdjWeightMap &adj_map);
  CoreEdge findNearestEdge(odb::dbBTerm* bTerm);
  std::string faninName(Macro *macro);
  int macroIndex(Macro *macro);
  bool macroIndexIsEdge(Macro *macro);

  void reportEdgePinCounts();

  ////////////////////////////////////////////////////////////////
    
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;

  // config filenames
  std::string globalConfig_;
  std::string localConfig_;

  bool isTiming_;

  // macro idx/idx pair -> give each
  std::vector<std::vector<int>> macroWeight;
  // macro Information
  std::vector<Macro> macroStor;
  // dbInst* --> macroStor's index
  std::unordered_map<odb::dbInst*, int> macroInstMap;

  // save LocalCfg into this structure
  std::unordered_map<std::string, MacroLocalInfo> macroLocalMap;

  // layout
  double lx_, ly_, ux_, uy_;
  double fenceLx_, fenceLy_, fenceUx_, fenceUy_;
  double siteSizeX_, siteSizeY_;
  double haloX_, haloY_;
  double channelX_, channelY_;
  double* netTable_;
  int verbose_;
  bool fenceRegionMode_;
  int solCount_;

  friend class Partition;
};

class Layout
{
 public:
  Layout();
  Layout(double lx, double ly, double ux, double uy);
  Layout(Layout& orig, Partition& part);

  double lx() const { return lx_; }
  double ly() const { return ly_; }
  double ux() const { return ux_; }
  double uy() const { return uy_; }

  void setLx(double lx);
  void setLy(double ly);
  void setUx(double ux);
  void setUy(double uy);

 private:
  double lx_, ly_, ux_, uy_;
};

}  // namespace mpl

