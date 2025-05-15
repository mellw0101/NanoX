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


/* ---------------------------------------------------------- Static function's ---------------------------------------------------------- */


static inline Ulong element_gridpos_hash(ElementGridpos pos) {
  Ulong hx = (Ulong)pos.x;
  Ulong hy = (Ulong)pos.y;
  return (hx ^ (hy * 0x9e3779b9UL + (hx << 6) + (hx >> 2)));
}

static inline Ulong element_grid_key(int x, int y) {
  return element_gridpos_hash((ElementGridpos){x, y});
}

static inline ElementGridpos element_gridpos_get(ElementGrid *const grid, float x, float y) {
  ASSERT(grid);
  return (ElementGridpos){(x / grid->cell_size), (y / grid->cell_size)};
}

static inline ElementGridpos element_grid_get_start(ElementGrid *const grid, Element *const e) {
  ASSERT(grid);
  ASSERT(e);
  return element_gridpos_get(grid, e->x, e->y);
}

static inline ElementGridpos element_grid_get_end(ElementGrid *const grid, Element *const e) {
  ASSERT(grid);
  ASSERT(e);
  return element_gridpos_get(grid, (e->x + e->width), (e->y + e->height));
}


/* ---------------------------------------------------------- Global function's ---------------------------------------------------------- */


ElementGrid *element_grid_create(int cell_size) {
  ElementGrid *grid;
  MALLOC_STRUCT(grid);
  grid->cell_size = cell_size;
  grid->map = hashmapnum_create_wfreefunc(cvec_free_void_ptr);
  return grid;
}

void element_grid_free(ElementGrid *const grid) {
  if (!grid) {
    return;
  }
  hashmapnum_free(grid->map);
  free(grid);
}

void element_grid_set(ElementGrid *const grid, Element *const e) {
  ASSERT(grid);
  ASSERT(e);
  ElementGridpos start;
  ElementGridpos end;
  Ulong key;
  CVec *vec;
  if (e->not_in_gridmap) {
    return;
  }
  start = element_grid_get_start(grid, e);
  end   = element_grid_get_end(grid, e);
  for (int x=start.x; x<=end.x; ++x) {
    for (int y=start.y; y<=end.y; ++y) {
      key = element_grid_key(x, y);
      vec = hashmapnum_get(grid->map, key);
      if (!vec) {
        vec = cvec_create();
        hashmapnum_insert(grid->map, key, vec);
      }
      cvec_push(vec, e);
    }
  }
}

void element_grid_remove(ElementGrid *const grid, Element *const e) {
  ASSERT(grid);
  ASSERT(e);
  ElementGridpos start;
  ElementGridpos end;
  Ulong key;
  CVec *vec;
  if (e->not_in_gridmap) {
    return;
  }
  start = element_grid_get_start(grid, e);
  end   = element_grid_get_end(grid, e);
  for (int x=start.x; x<=end.x; ++x) {
    for (int y=start.y; y<=end.y; ++y) {
      key = element_grid_key(x, y);
      vec = hashmapnum_get(grid->map, key);
      if (!vec) {
        continue;
      }
      cvec_remove_by_value(vec, e);
      if (!cvec_len(vec)) {
        hashmapnum_remove(grid->map, key);
      }
    }
  }
}

Element *element_grid_get(ElementGrid *const grid, float x, float y) {
  ASSERT(grid);
  ElementGridpos gridpos = element_gridpos_get(grid, x, y);
  Ulong key = element_grid_key(gridpos.x, gridpos.y);
  CVec *vec = hashmapnum_get(grid->map, key);
  Element *ret = NULL;
  Element *e;
  if (!vec) {
    return NULL;
  }
  for (int i=0; i<cvec_len(vec); ++i) {
    e = cvec_get(vec, i);
    if (e->hidden) {
      continue;
    }
    else if (x >= e->x && x <= (e->x + e->width) && y >= e->y && y <= (e->y + e->height)) {
      if ((ret && e->is_above) || !ret) {
        ret = e;
      }
    }
  }
  return ret;
}

bool element_grid_contains(ElementGrid *const grid, float x, float y) {
  ASSERT(grid);
  ElementGridpos gridpos = element_gridpos_get(grid, x, y);
  return hashmapnum_get(grid->map, element_grid_key(gridpos.x, gridpos.y));
}
