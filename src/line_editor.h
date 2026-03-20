#include <cstddef>
struct s_editor {
  char *buffer;
  size_t len;
  size_t capacity;
  // index inside the cursor where the cursor currently is
  size_t cursor;
};
