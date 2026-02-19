#include "mono_invoker.h"
#include "mono/metadata/object-forward.h"
#include "mono/metadata/object.h"
#include "mono/utils/mono-forward.h"

#include <mono/jit/jit.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/sgen-bridge.h>
#include <mono/metadata/threads.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
MonoDomain *s_root_domain = nullptr;

MonoDomain *get_root_domain() {
  if (s_root_domain)
    return s_root_domain;
  s_root_domain = mono_jit_init_version("LearnOpenGL", "v4.0.30319");
  if (!s_root_domain) {
    throw std::runtime_error("[Mono] Failed to initialize JIT");
  }
  mono_thread_attach(s_root_domain);
  return s_root_domain;
}

class domain_cleaner {
public:
  domain_cleaner() { s_root_domain = nullptr; }
  ~domain_cleaner() {
    if (s_root_domain) {
      // mono_jit_cleanup(s_root_domain);
    }
  }
};
static domain_cleaner s_domain_cleaner;

} // namespace

namespace mono_invoker {

class invoker_impl {
public:
  invoker_impl() { (void)get_root_domain(); }
  ~invoker_impl() { unload(); }

public:
  bool is_ready() const {
    return m_script_domain != nullptr && m_assembly != nullptr;
  }

  void load(const std::string &_assembly_path) {
    if (m_assembly) {
      throw std::runtime_error("[Mono] Assembly already loaded, unload first");
    }
    m_assembly_path = _assembly_path;

    std::string domain_name =
        "ScriptDomain_" + std::to_string(m_deduplication_count++);
    m_script_domain = mono_domain_create_appdomain(
        const_cast<char *>(domain_name.c_str()), nullptr);
    if (!m_script_domain) {
      throw std::runtime_error("[Mono] Failed to create script AppDomain");
    }

    MonoDomain *prev = mono_domain_get();
    mono_domain_set(m_script_domain, 1);
    mono_thread_attach(m_script_domain);

#ifdef __APPLE__
    // Create a shallow copy of the assembly with a unique name to avoid Mono
    // assembly cache.
    std::filesystem::path assembly_path_fs(_assembly_path);
    std::string copy_path_fs = assembly_path_fs.parent_path().string() +
                               "/ScriptAssembly_" +
                               std::to_string(m_deduplication_count++) +
                               assembly_path_fs.extension().string();
    std::ifstream fin(assembly_path_fs, std::ios::binary);
    std::ofstream fout(copy_path_fs, std::ios::binary);
    fout << fin.rdbuf();
    fin.close();
    fout.close();
    m_assembly = mono_assembly_open(copy_path_fs.c_str(), nullptr);
    std::filesystem::remove(copy_path_fs);
#else
    m_assembly = mono_assembly_open(_assembly_path.c_str(), nullptr);
#endif
    mono_domain_set(prev, 1);

    if (!m_assembly) {
      mono_domain_unload(m_script_domain);
      m_script_domain = nullptr;
      throw std::runtime_error("[Mono] Failed to load assembly: " +
                               _assembly_path);
    }
  }

  void unload() {
    if (!m_script_domain)
      return;
    MonoDomain *prev = mono_domain_get();
    if (prev == m_script_domain) {
      mono_domain_set(get_root_domain(), 1);
    }
    // Unload domain only; mono_domain_unload releases all assemblies in the
    // domain. Do not call mono_assembly_close to avoid double-free.
    mono_domain_unload(m_script_domain);
    m_script_domain = nullptr;
    m_assembly = nullptr;

    mono_gc_collect(mono_gc_max_generation());
    mono_gc_wait_for_bridge_processing();
  }

  bool invoke(const script_ncm &_ncm, const std::vector<void *> &_params) {
    if (!m_assembly || !m_script_domain)
      return false;

    MonoDomain *prev = mono_domain_get();
    mono_domain_set(m_script_domain, 1);
    bool ok = invoke_in_domain(_ncm, _params, nullptr);
    mono_domain_set(prev, 1);
    return ok;
  }

  bool invoke_in_domain(const script_ncm &_ncm,
                        const std::vector<void *> &_params,
                        MonoObject **_out_result) {
    if (!m_assembly)
      return false;

    MonoImage *image = mono_assembly_get_image(m_assembly);
    if (!image) {
      std::cerr << "[Mono] Failed to get image from assembly" << std::endl;
      return false;
    }

    MonoClass *klass =
        mono_class_from_name(image, _ncm.ns.c_str(), _ncm.cls.c_str());
    if (!klass) {
      std::cerr << "[Mono] Failed to get class from namespace: " << _ncm.ns
                << " and class: " << _ncm.cls << std::endl;
      return false;
    }

    int param_count = static_cast<int>(_params.size());
    MonoMethod *method =
        mono_class_get_method_from_name(klass, _ncm.md.c_str(), param_count);
    if (!method) {
      std::cerr << "[Mono] Failed to get method from class: " << _ncm.cls
                << " and method: " << _ncm.md
                << " (param_count=" << param_count << ")" << std::endl;
      return false;
    }

    return invoke_impl(method, const_cast<void **>(_params.data()), param_count,
                       _out_result);
  }

  bool invoke_r(const script_ncm &_ncm, const std::vector<void *> &_params,
                const invoke_result &_result_type, void *_result) {
    if (!m_assembly || !m_script_domain)
      return false;

    MonoDomain *prev = mono_domain_get();
    mono_domain_set(m_script_domain, 1);
    MonoObject *result_obj = nullptr;
    bool ok = invoke_in_domain(_ncm, _params, &result_obj);
    mono_domain_set(prev, 1);
    if (!ok || !result_obj)
      return false;
    if (_result_type == invoke_result::k_string) {
      MonoString *mstr = reinterpret_cast<MonoString *>(result_obj);
      char *cstr = mono_string_to_utf8(mstr);
      *(static_cast<std::string *>(_result)) = cstr ? cstr : "";
      if (cstr)
        mono_free(cstr);
    } else if (_result_type == invoke_result::k_integer) {
      *(static_cast<int *>(_result)) =
          *(int *)mono_object_unbox(result_obj);
    } else if (_result_type == invoke_result::k_float) {
      *(static_cast<float *>(_result)) =
          *(float *)mono_object_unbox(result_obj);
    }
    return true;
  }

private:
  bool invoke_impl(MonoMethod *_method, void **_params, int _param_count,
                   MonoObject **_result = nullptr) {
    if (!m_assembly)
      return false;
    MonoObject *exception = nullptr;
    if (_result) {
      *_result = mono_runtime_invoke(_method, nullptr, _params, &exception);
    } else {
      mono_runtime_invoke(_method, nullptr, _params, &exception);
    }
    if (exception) {
      MonoString *msg = mono_object_to_string(exception, nullptr);
      if (msg) {
        char *cstr = mono_string_to_utf8(msg);
        std::cerr << "[Mono] Exception: " << (cstr ? cstr : "unknown")
                  << std::endl;
        if (cstr) {
          mono_free(cstr);
        }
      }
      return false;
    }
    return true;
  }

private:
  MonoDomain *m_script_domain = nullptr;
  MonoAssembly *m_assembly = nullptr;
  std::string m_assembly_path;
  int m_deduplication_count = 0;
};

invoker::invoker() : m_impl(new invoker_impl()) {}
invoker::~invoker() { m_impl.reset(); }
bool invoker::is_ready() const { return m_impl->is_ready(); }

void invoker::load(const std::string &_assembly_path) {
  m_impl->load(_assembly_path);
}
void invoker::unload() { m_impl->unload(); }

bool invoker::invoke_impl(const script_ncm &_ncm,
                          const std::vector<void *> &_params) const {
  return m_impl->invoke(_ncm, _params);
}

bool invoker::invoke_impl(const script_ncm &_ncm,
                          const std::vector<void *> &_params,
                          invoke_result _result_type, void *_result) const {
  return m_impl->invoke_r(_ncm, _params, _result_type, _result);
}

} // namespace mono_invoker
