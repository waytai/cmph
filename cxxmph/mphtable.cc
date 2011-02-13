#include <limits>
#include <iostream>

using std::cerr;
using std::endl;

#include "mphtable.h"

using std::vector;

namespace {

static const uint8_t kUnassigned = 3;
// table used for looking up the number of assigned vertices to a 8-bit integer
static uint8_t kBdzLookupTable[] =
{
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 0
};

}  // anonymous namespace

namespace cxxmph {

const uint8_t MPHTable::valuemask[] = { 0xfc, 0xf3, 0xcf, 0x3f};

void MPHTable::clear() {
  // TODO(davi) impolement me
}
bool MPHTable::GenerateQueue(
    TriGraph* graph, vector<uint32_t>* queue_output) {
  uint32_t queue_head = 0, queue_tail = 0;
  uint32_t nedges = m_;
  uint32_t nvertices = n_;
  // Relies on vector<bool> using 1 bit per element
  vector<bool> marked_edge(nedges + 1, false);
  vector<uint32_t> queue(nvertices, 0);
  for (uint32_t i = 0; i < nedges; ++i) {
    const TriGraph::Edge& e = graph->edges()[i];
    if (graph->vertex_degree()[e[0]] == 1 ||
        graph->vertex_degree()[e[1]] == 1 ||
        graph->vertex_degree()[e[2]] == 1) {
      if (!marked_edge[i]) {
        queue[queue_head++] = i;
        marked_edge[i] = true;
      }
    }
  }
  /*
  for (unsigned int i = 0; i < marked_edge.size(); ++i) {
    cerr << "vertex with degree " << static_cast<uint32_t>(graph->vertex_degree()[i]) << " marked " << marked_edge[i] << endl;
  }
  for (unsigned int i = 0; i < queue.size(); ++i) {
    cerr << "vertex " << i << " queued at " << queue[i] << endl;
  }
  */
  // At this point queue head is the number of edges touching at least one
  // vertex of degree 1.
  // cerr << "Queue head " << queue_head << " Queue tail " << queue_tail << endl;
  // graph->DebugGraph();
  while (queue_tail != queue_head) {
    uint32_t current_edge = queue[queue_tail++];
    graph->RemoveEdge(current_edge);
    const TriGraph::Edge& e = graph->edges()[current_edge];
    for (int i = 0; i < 3; ++i) {
      uint32_t v = e[i];
      if (graph->vertex_degree()[v] == 1) {
        uint32_t first_edge = graph->first_edge()[v];
        if (!marked_edge[first_edge]) {
          queue[queue_head++] = first_edge;
          marked_edge[first_edge] = true;
        }
      }
    }
  }
  /*
  for (unsigned int i = 0; i < queue.size(); ++i) {
    cerr << "vertex " << i << " queued at " << queue[i] << endl;
  }
  */
  int cycles = queue_head - nedges;
  if (cycles == 0) queue.swap(*queue_output);
  return cycles == 0;
}

void MPHTable::Assigning(
    const vector<TriGraph::Edge>& edges, const vector<uint32_t>& queue) {
  uint32_t current_edge = 0;
  vector<bool> marked_vertices(n_ + 1);
  // Initialize vector of half nibbles with all bits set.
  uint32_t sizeg = static_cast<uint32_t>(ceil(n_/4.0));
  vector<uint8_t>(sizeg, std::numeric_limits<uint8_t>::max()).swap(g_);

  uint32_t nedges = m_;  // for legibility
  for (int i = nedges - 1; i + 1 >= 1; --i) {
    current_edge = queue[i];
    const TriGraph::Edge& e = edges[current_edge];
    // cerr << "B: " << e[0] << " " << e[1] << " " << e[2] << " -> "
    //    << get_2bit_value(g_, e[0]) << " "
    //    << get_2bit_value(g_, e[1]) << " "
    //    << get_2bit_value(g_, e[2]) << " edge " << current_edge  << endl;
    if (!marked_vertices[e[0]]) {
      if (!marked_vertices[e[1]]) {
        set_2bit_value(&g_, e[1], kUnassigned);
        marked_vertices[e[1]] = true;
      }
      if (!marked_vertices[e[2]]) {
        set_2bit_value(&g_, e[2], kUnassigned);
	assert(marked_vertices.size() > e[2]);
        marked_vertices[e[2]] = true;
      }
      set_2bit_value(&g_, e[0], (6 - (get_2bit_value(g_, e[1]) + get_2bit_value(g_, e[2]))) % 3);
      marked_vertices[e[0]] = true;
    } else if (!marked_vertices[e[1]]) {
      if (!marked_vertices[e[2]]) {
        set_2bit_value(&g_, e[2], kUnassigned);
        marked_vertices[e[2]] = true;
      }
      set_2bit_value(&g_, e[1], (7 - (get_2bit_value(g_, e[0]) + get_2bit_value(g_, e[2]))) % 3);
      marked_vertices[e[1]] = true;
    } else {
      set_2bit_value(&g_, e[2], (8 - (get_2bit_value(g_, e[0]) + get_2bit_value(g_, e[1]))) % 3);
      marked_vertices[e[2]] = true;
    }
    // cerr << "A: " << e[0] << " " << e[1] << " " << e[2] << " -> "
    //    << get_2bit_value(g_, e[0]) << " "
    //    << get_2bit_value(g_, e[1]) << " "
    //    << get_2bit_value(g_, e[2]) << " " << endl;
  }
}

void MPHTable::Ranking() {
  uint32_t nbytes_total = static_cast<uint32_t>(ceil(n_ / 4.0));
  uint32_t size = k_ >> 2U;
  uint32_t ranktablesize = static_cast<uint32_t>(
      ceil(n_ / static_cast<double>(k_)));
  // TODO(davi) Change swap of member classes for resize + memset to avoid
  // fragmentation
  vector<uint32_t> (ranktablesize).swap(ranktable_);;
  uint32_t offset = 0;
  uint32_t count = 0;
  uint32_t i = 1;
  while (1) {
    if (i == ranktable_.size()) break;
    uint32_t nbytes = size < nbytes_total ? size : nbytes_total;
    for (uint32_t j = 0; j < nbytes; ++j) count += kBdzLookupTable[g_[offset + j]];
    ranktable_[i] = count;
    offset += nbytes;
    nbytes_total -= size;
    ++i;
  }
}

uint32_t MPHTable::Rank(uint32_t vertex) const {
  uint32_t index = vertex >> b_;
  uint32_t base_rank = ranktable_[index];
  uint32_t beg_idx_v = index << b_;
  uint32_t beg_idx_b = beg_idx_v >> 2;
  uint32_t end_idx_b = vertex >> 2;
  while (beg_idx_b < end_idx_b) base_rank += kBdzLookupTable[g_[beg_idx_b++]];
  beg_idx_v = beg_idx_b << 2;
  // cerr << "beg_idx_v: " << beg_idx_v << endl;
  // cerr << "base rank: " << base_rank << endl;

  //cerr << "G: ";
  // for (unsigned int i = 0; i < n_; ++i) {
  //  cerr << get_2bit_value(g_, i) << " ";
  //}
  // cerr << endl;
  while (beg_idx_v < vertex) {
    if (get_2bit_value(g_, beg_idx_v) != kUnassigned) ++base_rank;
    ++beg_idx_v;
  }
  // cerr << "Base rank: " << base_rank << endl;
  return base_rank;
}

}  // namespace cxxmph
