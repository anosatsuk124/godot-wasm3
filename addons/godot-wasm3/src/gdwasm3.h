#ifndef GDWASM3_H
#define GDWASM3_H

#include <_types/_uint32_t.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <wasm3.h>

namespace godot {

class GDWasm3 : public RefCounted {
  GDCLASS(GDWasm3, RefCounted)

protected:
  static void _bind_methods();

public:
  GDWasm3();
  ~GDWasm3();

  void instantiate(const uint32_t stack_size_in_bytes,
                   PackedByteArray bytecode);

  Variant call(StringName fn_name, Array args);

  void _init();

  void _reset();

  IM3Environment *env;
  IM3Runtime *runtime;
  IM3Module *module;
};

} // namespace godot

#endif
