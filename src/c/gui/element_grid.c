/** @file gui/element_grid.c

  @author  Melwin Svensson.
  @date    15-5-2025.

 */
#include "../../include/c_proto.h"


/* ---------------------------------------------------------- Variable's ---------------------------------------------------------- */


ElementGrid *element_grid = NULL;


/* ---------------------------------------------------------- Struct's ---------------------------------------------------------- */


struct ElementGrid {
  int cell_size;
  HashMapNum *map;
};

typedef struct {
  float x;
  float y;
  Element *ret;
} GetPackage;


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static inline Ulong element_gridpos_hash(ElementGridpos pos) {
  Ulong hx = (Ulong)pos.x;
  Ulong hy = (Ulong)pos.y;
  return (hx ^ (hy * 0x9e3779b9UL + (hx << 6) + (hx >> 2)));
}

static inline Ulong element_grid_key(int x, int y) {
  return element_gridpos_hash((ElementGridpos){x, y});
}

static inline ElementGridpos element_gridpos_get(float x, float y) {
  ASSERT(element_grid);
  return (ElementGridpos){(x / element_grid->cell_size), (y / element_grid->cell_size)};
}

static inline ElementGridpos element_grid_get_start(Element *const e) {
  ASSERT(element_grid);
  ASSERT(e);
  return element_gridpos_get(e->x, e->y);
}

static inline ElementGridpos element_grid_get_end(Element *const e) {
  ASSERT(element_grid);
  ASSERT(e);
  return element_gridpos_get((e->x + e->width), (e->y + e->height));
}

static inline void element_grid_get_action(Ulong _UNUSED key, void *value, void *data) {
  ASSERT(value);
  Element *e = value;
  GetPackage *p;
  if (e->hidden) {
    return;
  }
  p = data;
  /* Ensure the given position actually falls inside the element. */
  if ((p->x >= e->x && p->x <= (e->x + e->width) && p->y >= e->y && p->y <= (e->y + e->height))
   /* Also, ensure the layers are respected. */
   && (!p->ret || (p->ret->layer < e->layer || (p->ret->layer == e->layer && !p->ret->is_above && e->is_above)))) {
    p->ret = e;
  }
  // if (p->x >= e->x && p->x <= (e->x + e->width) && p->y >= e->y && p->y <= (e->y + e->height)) {
  //   if (!p->ret || (p->ret && (p->ret->layer < e->layer || (p->ret->layer == e->layer && !p->ret->is_above && e->is_above)))) {
  //     p->ret = e;
  //   }
  // }
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


void element_grid_create(int cell_size) {
  MALLOC_STRUCT(element_grid);
  element_grid->cell_size = cell_size;
  element_grid->map = hashmapnum_create_wfreefunc(hashmapnum_free_void_ptr);
}

void element_grid_free(void) {
  if (!element_grid) {
    return;
  }
  hashmapnum_free(element_grid->map);
  free(element_grid);
  element_grid = NULL;
}

void element_grid_set(Element *const e) {
  ASSERT(element_grid);
  ASSERT(e);
  ElementGridpos start;
  ElementGridpos end;
  Ulong key;
  HashMapNum *cellmap;
  if (e->not_in_gridmap) {
    return;
  }
  start = element_grid_get_start(e);
  end   = element_grid_get_end(e);
  for (int x=start.x; x<=end.x; ++x) {
    for (int y=start.y; y<=end.y; ++y) {
      key     = element_grid_key(x, y);
      cellmap = hashmapnum_get(element_grid->map, key);
      if (!cellmap) {
        cellmap = hashmapnum_create();
        hashmapnum_insert(element_grid->map, key, cellmap);
      }
      hashmapnum_insert(cellmap, (Ulong)e, e);
    }
  }
}

void element_grid_remove(Element *const e) {
  ASSERT(element_grid);
  ASSERT(e);
  ElementGridpos start;
  ElementGridpos end;
  Ulong key;
  HashMapNum *cellmap;
  if (e->not_in_gridmap) {
    return;
  }
  start = element_grid_get_start(e);
  end   = element_grid_get_end(e);
  for (int x=start.x; x<=end.x; ++x) {
    for (int y=start.y; y<=end.y; ++y) {
      key = element_grid_key(x, y);
      cellmap = hashmapnum_get(element_grid->map, key);
      if (!cellmap) {
        continue;
      }
      hashmapnum_remove(cellmap, (Ulong)e);
      // if (!hashmapnum_size(cellmap)) {
      //   hashmapnum_remove(grid->map, key);
      // }
    }
  }
}

Element *element_grid_get(float x, float y) {
  ASSERT(element_grid);
  ElementGridpos gridpos = element_gridpos_get(x, y);
  Ulong key = element_grid_key(gridpos.x, gridpos.y);
  HashMapNum *cellmap = hashmapnum_get(element_grid->map, key);
  GetPackage *p;
  Element *ret;
  if (!cellmap) {
    return NULL;
  }
  p = xmalloc(sizeof(*p));
  p->x   = x;
  p->y   = y;
  p->ret = NULL;
  hashmapnum_forall_wdata(cellmap, element_grid_get_action, p);
  ret = p->ret;
  free(p);
  return ret;
}

bool element_grid_contains(float x, float y) {
  ASSERT(element_grid);
  ElementGridpos gridpos = element_gridpos_get(x, y);
  return hashmapnum_get(element_grid->map, element_grid_key(gridpos.x, gridpos.y));
}

// ElementGrid *element_grid_create(int cell_size) {
//   ElementGrid *grid;
//   MALLOC_STRUCT(grid);
//   grid->cell_size = cell_size;
//   grid->map = hashmapnum_create_wfreefunc(cvec_free_void_ptr);
//   return grid;
// }
// void element_grid_set(ElementGrid *const grid, Element *const e) {
//   ASSERT(grid);
//   ASSERT(e);
//   ElementGridpos start;
//   ElementGridpos end;
//   Ulong key;
//   CVec *vec;
//   if (e->not_in_gridmap) {
//     return;
//   }
//   start = element_grid_get_start(grid, e);
//   end   = element_grid_get_end(grid, e);
//   for (int x=start.x; x<=end.x; ++x) {
//     for (int y=start.y; y<=end.y; ++y) {
//       key = element_grid_key(x, y);
//       vec = hashmapnum_get(grid->map, key);
//       if (!vec) {
//         vec = cvec_create();
//         hashmapnum_insert(grid->map, key, vec);
//       }
//       cvec_push(vec, e);
//     }
//   }
// }
// void element_grid_remove(ElementGrid *const grid, Element *const e) {
//   ASSERT(grid);
//   ASSERT(e);
//   TIMER_START(timer);
//   ElementGridpos start;
//   ElementGridpos end;
//   Ulong key;
//   CVec *vec;
//   if (e->not_in_gridmap) {
//     return;
//   }
//   start = element_grid_get_start(grid, e);
//   end   = element_grid_get_end(grid, e);
//   for (int x=start.x; x<=end.x; ++x) {
//     for (int y=start.y; y<=end.y; ++y) {
//       key = element_grid_key(x, y);
//       vec = hashmapnum_get(grid->map, key);
//       if (!vec) {
//         continue;
//       }
//       cvec_remove_by_value(vec, e);
//       if (!cvec_len(vec)) {
//         hashmapnum_remove(grid->map, key);
//       }
//     }
//   }
//   TIMER_END(timer, ms);
//   TIMER_PRINT(ms);
// }
// Element *element_grid_get(ElementGrid *const grid, float x, float y) {
//   ASSERT(grid);
//   TIMER_START(timer);
//   ElementGridpos gridpos = element_gridpos_get(grid, x, y);
//   Ulong key = element_grid_key(gridpos.x, gridpos.y);
//   CVec *vec = hashmapnum_get(grid->map, key);
//   Element *ret = NULL;
//   Element *e;
//   if (!vec) {
//     return NULL;
//   }
//   for (int i=0; i<cvec_len(vec); ++i) {
//     e = cvec_get(vec, i);
//     if (e->hidden) {
//       continue;
//     }
//     else if (x >= e->x && x <= (e->x + e->width) && y >= e->y && y <= (e->y + e->height)) {
//       if ((ret && e->is_above) || !ret) {
//         ret = e;
//       }
//     }
//   }
//   TIMER_END(timer, ms);
//   TIMER_PRINT(ms);
//   return ret;
// }
