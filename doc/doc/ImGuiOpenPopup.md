Mark a named popup as open.

Popup windows are closed when the user:

* Clicks outside of their client rect,
* Activates a pressable item,
* [ImGuiCloseCurrentPopup] is called within a [ImGuiBeginPopup]/[ImGuiEndPopup] block.

Popup identifiers are relative to the current ID stack so [ImGuiOpenPopup] and [ImGuiBeginPopup] need to be at the same level of the ID stack.