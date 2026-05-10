#pragma once

#include "doc_input_handler.h"

#include "tests/cad/database/handle.h"

#include "imgui.h"

namespace toy_cad::interaction {

/** Per-row input after drawing a tree header / leaf (last item). */
struct tree_item_signals {
  bool left_clicked = false;
  bool right_clicked = false;
};

/** Collapsible branch via ImGui::TreeNodeEx — caller must ImGui::TreePop() when `open`. */
struct tree_branch_outcome {
  bool open = false;
  tree_item_signals signals{};
};

tree_branch_outcome tree_branch(const char *label_id,
                                ImGuiTreeNodeFlags flags = 0);

/** Selection highlight + explorer click routing via doc_input_handler. */
tree_branch_outcome tree_branch(const char *label_id, ImGuiTreeNodeFlags flags,
                                handle item, doc_input_handler &doc);

/** Leaf row (bullet, no TreePop). */
struct tree_leaf_outcome {
  tree_item_signals signals{};
};

tree_leaf_outcome tree_leaf(const char *label_id,
                            ImGuiTreeNodeFlags extra_flags = 0);

tree_leaf_outcome tree_leaf(const char *label_id, ImGuiTreeNodeFlags extra_flags,
                            handle item, doc_input_handler &doc);

/**
 * Call immediately after tree_branch / tree_leaf on the same line.
 * If conversion to bool is true, draw menu items then let destructor EndPopup.
 */
class tree_item_context_menu {
public:
  explicit tree_item_context_menu(const char *popup_str_id = nullptr);
  tree_item_context_menu(const tree_item_context_menu &) = delete;
  tree_item_context_menu &operator=(const tree_item_context_menu &) = delete;
  ~tree_item_context_menu();
  explicit operator bool() const { return active_; }

private:
  bool active_;
};

inline tree_branch_outcome tree_branch(const char *label_id,
                                       const ImGuiTreeNodeFlags flags) {
  tree_branch_outcome out;
  out.open = ImGui::TreeNodeEx(label_id, flags);
  out.signals.left_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
  out.signals.right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
  return out;
}

inline tree_branch_outcome tree_branch(const char *label_id,
                                       ImGuiTreeNodeFlags flags,
                                       handle item, doc_input_handler &doc) {
  tree_branch_outcome out;
  if (item.valid() && doc.is_selected(item)) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  out.open = ImGui::TreeNodeEx(label_id, flags);
  out.signals.left_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
  out.signals.right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
  if (item.valid() && out.signals.left_clicked) {
    doc.on_explorer_item_left_click(item);
  }
  return out;
}

inline tree_leaf_outcome tree_leaf(const char *label_id,
                                   const ImGuiTreeNodeFlags extra_flags) {
  const ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
      ImGuiTreeNodeFlags_Bullet | extra_flags;
  ImGui::TreeNodeEx(label_id, flags);
  tree_leaf_outcome out;
  out.signals.left_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
  out.signals.right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
  return out;
}

inline tree_leaf_outcome tree_leaf(const char *label_id,
                                   ImGuiTreeNodeFlags extra_flags,
                                   handle item, doc_input_handler &doc) {
  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
      ImGuiTreeNodeFlags_Bullet | extra_flags;
  if (item.valid() && doc.is_selected(item)) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  ImGui::TreeNodeEx(label_id, flags);
  tree_leaf_outcome out;
  out.signals.left_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
  out.signals.right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
  if (item.valid() && out.signals.left_clicked) {
    doc.on_explorer_item_left_click(item);
  }
  return out;
}

inline tree_item_context_menu::tree_item_context_menu(const char *popup_str_id)
    : active_(ImGui::BeginPopupContextItem(popup_str_id)) {}

inline tree_item_context_menu::~tree_item_context_menu() {
  if (active_) {
    ImGui::EndPopup();
  }
}

} // namespace toy_cad::interaction
