#pragma once

#include "../definitions.h"

class RendrEngine {
 private:
  static RendrEngine *_instance;

  static void _destroy(void) noexcept;
  void _render_bracket(int row, linestruct *line);
  void _brackets(void);

 public:
  static RendrEngine *instance(void) noexcept;
  void whole_editwin(void);
};