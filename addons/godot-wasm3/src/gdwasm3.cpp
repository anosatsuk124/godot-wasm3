#include "gdwasm3.h"
#include <_types/_uint32_t.h>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <wasm3.h>

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GDWasm3::_bind_methods() {
  // ClassDB::bind_method()
  ClassDB::bind_method(
      D_METHOD("instantiate", "stack_size_in_bytes", "bytecode"),
      &GDWasm3::instantiate);
  ClassDB::bind_method(D_METHOD("call", "fn_name", "args"), &GDWasm3::call);
  ClassDB::bind_method(D_METHOD("_init"), &GDWasm3::_init);
  ClassDB::bind_method(D_METHOD("_reset"), &GDWasm3::_reset);
}

void GDWasm3::_init() {
  this->_reset();
  *this->env = m3_NewEnvironment();
}

void GDWasm3::_reset() {
  if (this->env != nullptr) {
    m3_FreeEnvironment(*this->env);
  }
  if (this->runtime != nullptr) {
    m3_FreeRuntime(*this->runtime);
  }
  if (this->module != nullptr) {
    m3_FreeModule(*this->module);
  }

  this->env = nullptr;
  this->runtime = nullptr;
  this->module = nullptr;
}

GDWasm3::GDWasm3() { this->_init(); }

GDWasm3::~GDWasm3() { this->_reset(); }

void GDWasm3::instantiate(const uint32_t stack_size_in_bytes,
                          PackedByteArray bytecode) {
  *this->runtime = m3_NewRuntime(*this->env, stack_size_in_bytes, nullptr);

  m3_ParseModule(*this->env, this->module, bytecode.ptr(), bytecode.size());
}

Variant GDWasm3::call(StringName fn_name, Array args) {
  IM3Function *fn;
  m3_FindFunction(fn, *this->runtime, fn_name.c_escape().utf8().ptr());

  uint32_t fn_arg_count = m3_GetArgCount(*fn);

  if (fn_arg_count != args.size()) {
    ERR_PRINT("Argument count mismatch.");
    return Variant();
  }

  const char **argv;
  for (uint32_t i = 0; i < fn_arg_count; i++) {
    argv[i] = reinterpret_cast<char *>(&args[i]);
  }

  M3Result m3_result = m3_CallArgv(*fn, fn_arg_count, argv);

  return Variant(m3_result);
}
