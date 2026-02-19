
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

  void load(const std::string &assembly_path) {
    if (m_assembly) {
      throw std::runtime_error("[Mono] Assembly already loaded, unload first");
    }
    m_assembly_path = assembly_path;

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
    // Technically, create a shallow copy of the assembly with random name to
    // avoid mono cache.
    std::filesystem::path assembly_path_fs(assembly_path);
    std::string copy_path_fs = assembly_path_fs.parent_path().string() +
                               "\\"
                               "/ScriptAssembly_" +
                               std::to_string(m_deduplication_count++) +
                               assembly_path_fs.extension().string();
    std::ifstream fin(assembly_path_fs, std::ios::binary);
    std::ofstream fout(copy_path_fs, std::ios::binary);
    fout << fin.rdbuf();
    fin.close();
    fout.close();
    m_assembly = mono_assembly_open(copy_path_fs.c_str(), nullptr);
    // Delete the copy file after loading.
    std::filesystem::remove(copy_path_fs);
#else
    m_assembly = mono_assembly_open(assembly_path.c_str(), nullptr);
#endif
    mono_domain_set(prev, 1);

    if (!m_assembly) {
      mono_domain_unload(m_script_domain);
      m_script_domain = nullptr;
      throw std::runtime_error("[Mono] Failed to load assembly: " +
                               assembly_path);
    }
  }

  void unload() {
    if (!m_script_domain)
      return;
    MonoDomain *prev = mono_domain_get();
    if (prev == m_script_domain) {
      mono_domain_set(get_root_domain(), 1);
    }
    // 只卸载域即可：mono_domain_unload 会释放域内所有程序集，不要再调
    // mono_assembly_close， 否则会双重释放导致崩溃。
    // mono_assembly_close(m_assembly);
    mono_domain_unload(m_script_domain);
    m_script_domain = nullptr;
    m_assembly = nullptr;

    // 强制进行垃圾回收，尝试清理残留
    mono_gc_collect(mono_gc_max_generation());
    mono_gc_wait_for_bridge_processing(); // 等待桥接处理完成
  }

  bool invoke(const script_ncm &ncm, const std::vector<void *> &params) {
    if (!m_assembly || !m_script_domain)
      return false;

    MonoDomain *prev = mono_domain_get();
    mono_domain_set(m_script_domain, 1);
    bool ok = invoke_in_domain(ncm, params, nullptr);
    mono_domain_set(prev, 1);
    return ok;
  }

  bool invoke_in_domain(const script_ncm &ncm,
                        const std::vector<void *> &params,
                        MonoObject **out_result) {
    if (!m_assembly)
      return false;

    MonoImage *image = mono_assembly_get_image(m_assembly);
    if (!image) {
      std::cerr << "[Mono] Failed to get image from assembly" << std::endl;
      return false;
    }

    MonoClass *klass =
        mono_class_from_name(image, ncm.ns.c_str(), ncm.cls.c_str());
    if (!klass) {
      std::cerr << "[Mono] Failed to get class from namespace: " << ncm.ns
                << " and class: " << ncm.cls << std::endl;
      return false;
    }

    int param_count = static_cast<int>(params.size());
    MonoMethod *method =
        mono_class_get_method_from_name(klass, ncm.md.c_str(), param_count);
    if (!method) {
      std::cerr << "[Mono] Failed to get method from class: " << ncm.cls
                << " and method: " << ncm.md << " (param_count=" << param_count
                << ")" << std::endl;
      return false;
    }

    return invoke_impl(method, const_cast<void **>(params.data()), param_count,
                       out_result);
  }

  bool invoke_r(const script_ncm &ncm, const std::vector<void *> &params,
                const invoke_result &result_type, void *result) {
    if (!m_assembly || !m_script_domain)
      return false;

    MonoDomain *prev = mono_domain_get();
    mono_domain_set(m_script_domain, 1);
    MonoObject *result_obj = nullptr;
    bool ok = invoke_in_domain(ncm, params, &result_obj);
    mono_domain_set(prev, 1);
    if (!ok || !result_obj)
      return false;
    if (result_type == invoke_result::k_string) {
      MonoString *mstr = reinterpret_cast<MonoString *>(result_obj);
      char *cstr = mono_string_to_utf8(mstr);
      *(static_cast<std::string *>(result)) = cstr ? cstr : "";
      if (cstr)
        mono_free(cstr);
    } else if (result_type == invoke_result::k_integer) {
      *(static_cast<int *>(result)) = *(int *)mono_object_unbox(result_obj);
    } else if (result_type == invoke_result::k_float) {
      *(static_cast<float *>(result)) = *(float *)mono_object_unbox(result_obj);
    }
    return true;
  }

private:
  bool invoke_impl(MonoMethod *method, void **params, int param_count,
                   MonoObject **result = nullptr) {
    if (!m_assembly) {
      return false;
    }
    MonoObject *exception = nullptr;
    if (result) {
      *result = mono_runtime_invoke(method, nullptr, params, &exception);
    } else {
      mono_runtime_invoke(method, nullptr, params, &exception);
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

void invoker::load(const std::string &assembly_path) {
  m_impl->load(assembly_path);
}
void invoker::unload() { m_impl->unload(); }

bool invoker::invoke_impl(const script_ncm &ncm,
                          const std::vector<void *> &params) const {
  return m_impl->invoke(ncm, params);
}

bool invoker::invoke_impl(const script_ncm &ncm,
                          const std::vector<void *> &params,
                          invoke_result result_type, void *result) const {
  return m_impl->invoke_r(ncm, params, result_type, result);
}

} // namespace mono_invoker